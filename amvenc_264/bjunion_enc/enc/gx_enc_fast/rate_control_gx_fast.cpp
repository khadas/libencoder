/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */



//#define LOG_NDEBUG 0
#define LOG_TAG "FASTGX_RC"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include "enc_define.h"
#include "gxvenclib_fast.h"
#include "rate_control_gx_fast.h"
#ifdef __ANDROID__
#include <cutils/properties.h>
//#include <utils/Log.h>
#endif
#define IDR_SCALER_RATIO 4
#define BITRATE_URGENT_TH 20
#define BITRATE_URGENT_MID 10
#define BITRATE_URGENT_INC_STEP3 5
#define BITRATE_URGENT_INC_STEP2 2
#define BITRATE_URGENT_INC_STEP1 1

#define BITRATE_URGENT_DEC_STEP3 3
#define BITRATE_URGENT_DEC_STEP2 2
#define BITRATE_URGENT_DEC_STEP1 1

static void BitrateScale(GxFastEncRateControl *rateCtrl) {
    int bitsPerframe = (int) (rateCtrl->bitRate / rateCtrl->frame_rate);

    float ratio = 1.0;
    if (rateCtrl->BitrateScale == false)
        return;

    if (rateCtrl->bitRate >= 5000000)
        return;

    if (rateCtrl->frame_rate < 10.0)
        ratio = 1.3;
    else if (rateCtrl->frame_rate < 15.0)
        ratio = 1.2;
    else if (rateCtrl->frame_rate < 20.0)
        ratio = 1.1;

    if (bitsPerframe < 80000) {
        bitsPerframe = bitsPerframe * 11 / 10;
        bitsPerframe = (int) 80000 * ratio;
    } else if (bitsPerframe < 100000) {
        bitsPerframe = bitsPerframe * 16 / 10;
        bitsPerframe = (int) bitsPerframe * ratio;
    } else if (bitsPerframe < 150000) {
        bitsPerframe = bitsPerframe * 12 / 10;
        bitsPerframe = (int) bitsPerframe * ratio;
    }

    if (bitsPerframe >= 180000)
        bitsPerframe = 180000;

    rateCtrl->bitRate = (int) (bitsPerframe * rateCtrl->frame_rate);

    return;
}

static void updateRateControl(GxFastEncRateControl *rateCtrl, bool IDR) {
    if (IDR)
        rateCtrl->buffer_fullness += rateCtrl->Rc/*/IDR_SCALER_RATIO*/;
    else
        rateCtrl->buffer_fullness += rateCtrl->Rc;

    VLOG(NONE,"new: buffer_fullness:%lld numFrameBits:%d", rateCtrl->buffer_fullness, rateCtrl->Rc);
    rateCtrl->encoded_frames++;
    /**scene detact**/
    rateCtrl->skip_next_frame = 0;

    if (IDR) {
        rateCtrl->last_IDR_bits = rateCtrl->Rc;
        rateCtrl->last_pframe_bits = 0x7fffffff;
    } else {
        rateCtrl->last_pframe_bits = rateCtrl->Rc;
    }

    return;
}

#define ADJ_QP_MIN 16
#define ADJ_QP_MAX 40

AMVEnc_Status GxFastRCUpdateFrame(void *dev, void *rc, bool IDR, int* skip_num, int numFrameBits) {
    gx_fast_enc_drv_t *p = (gx_fast_enc_drv_t *) dev;
    GxFastEncRateControl *rateCtrl = (GxFastEncRateControl *) rc;
    AMVEnc_Status status = AMVENC_SUCCESS;
    int diff_BTCounter;
    int32_t qp_max;

    *skip_num = 0;
    if (rateCtrl->rcEnable == true) {
        rateCtrl->Rc = numFrameBits;
        if (((int) rateCtrl->estimate_fps > 25) && (rateCtrl->bitsPerFrame / p->src.mb_width / p->src.mb_height < 20))
            qp_max = ADJ_QP_MAX + 10;
        else
            qp_max = ADJ_QP_MAX;

        if (p->max_qp_count > 40) {
            if ((uint32_t)(p->src.mb_width * p->src.mb_height) < 3 * p->qp_stic.i_count)
                qp_max = ADJ_QP_MAX + 5;
        }

        if (p->cbr_hw == true) {
//            char prop[PROPERTY_VALUE_MAX];
//            int gts_test = 0;
//            int isReenc = 1;
//#ifdef SUPPORT_STANDARD_PROP
//            if (property_get("vendor.hw.encoder.bitrate.test", prop, NULL) > 0)
//#else
//            if (property_get("hw.encoder.bitrate.test", prop, NULL) > 0)
//#endif
//                sscanf(prop, "%d", &gts_test);

//            int media_gts_test = 0;
//            memset(prop, 0, sizeof(prop));
//#ifdef SUPPORT_STANDARD_PROP
//            if (property_get("vendor.media.encoder.bitrate.test", prop, NULL) > 0)
//#else
//            if (property_get("media.encoder.bitrate.test", prop, NULL) > 0)
//#endif
//                sscanf(prop, "%d", &media_gts_test);

//#ifdef SUPPORT_STANDARD_PROP
//            if (property_get("vendor.hw.encoder.reencode.enable", prop, NULL) > 0)
//#else
//            if (property_get("hw.encoder.reencode.enable", prop, NULL) > 0)
//#endif
//                sscanf(prop, "%d", &isReenc);

            if (rateCtrl->bitsPerFrame / p->src.mb_width / p->src.mb_height < 16)
                qp_max = 38;
            else
                qp_max = 32;

//            if (gts_test > 0 || media_gts_test > 0)
//                qp_max = 51;

            /* compression ratio large than 1.5M 720p and result is large than 1.7 times target */
//            if (isReenc == 1) {
                if ((rateCtrl->bitsPerFrame / p->src.mb_width / p->src.mb_height > 14)
                        && (numFrameBits > p->target * 17 / 10)) {
                    status = AMVENC_REENCODE_PICTURE;
                    if ((uint32_t)(p->src.mb_width * p->src.mb_height) < 3 * p->qp_stic.i_count)
                        p->quant += 6 * numFrameBits / p->target;
                }
//            }
        } else {
            if (qp_max > 48)
                qp_max = 48;
        }

        if (!rateCtrl->BitrateScale && !p->cabac_mode) {
            if (p->bitrate_urgent_mode) {
                qp_max = 51;
            } else if (p->bitrate_urgent_cnt > BITRATE_URGENT_MID) {
                int temp = p->bitrate_urgent_cnt - BITRATE_URGENT_MID;

                if ((temp * 3) >= BITRATE_URGENT_MID)
                    qp_max += 2;
                else if ((temp * 2) >= BITRATE_URGENT_MID)
                    qp_max += 3;
                else if (((temp * 10) /7) >= BITRATE_URGENT_MID)
                    qp_max += 4;
                if (qp_max > 51)
                    qp_max = 51;
            }
        }
        if ((unsigned int) numFrameBits > p->target * 3) {
            VLOG(NONE,"base qp + 10:%d, bu_cnt:%d,qp_max:%d", p->quant, p->bitrate_urgent_cnt,qp_max);
            p->quant += 10;
            if (p->quant > qp_max)
                p->quant = qp_max;
            if (!p->bitrate_urgent_mode)
                p->bitrate_urgent_cnt += BITRATE_URGENT_INC_STEP3;
        } else if ((unsigned int) numFrameBits > p->target * 2) {
            VLOG(NONE,"base qp + 6:%d, bu_cnt:%d,qp_max:%d", p->quant, p->bitrate_urgent_cnt,qp_max);
            p->quant += 6;
            if (p->quant > qp_max)
                p->quant = qp_max;
            if (!p->bitrate_urgent_mode)
                p->bitrate_urgent_cnt += BITRATE_URGENT_INC_STEP2;
        } else if ((unsigned int) numFrameBits > p->target * 3 / 2) {
            VLOG(NONE,"base qp + 3:%d, bu_cnt:%d,qp_max:%d", p->quant, p->bitrate_urgent_cnt,qp_max);
            p->quant += 3;
            if (p->quant > qp_max)
                p->quant = qp_max;
            if (!p->bitrate_urgent_mode)
                p->bitrate_urgent_cnt += BITRATE_URGENT_INC_STEP1;
        } else if ((unsigned int) numFrameBits > p->target * 11 / 10) {
            VLOG(NONE,"base qp++:%d, bu_cnt:%d,qp_max:%d", p->quant, p->bitrate_urgent_cnt,qp_max);
            p->quant++;
            if (p->quant > qp_max)
                p->quant = qp_max;
        } else if (((unsigned int) numFrameBits < p->target / 2) && p->quant > 15) {
            VLOG(NONE,"base qp - 4:%d, bu_cnt:%d,qp_max:%d", p->quant, p->bitrate_urgent_cnt,qp_max);
            p->quant -= 4;
            if (p->quant < ADJ_QP_MIN)
                p->quant = ADJ_QP_MIN;
            if (p->bitrate_urgent_cnt > BITRATE_URGENT_MID)
                p->bitrate_urgent_cnt = BITRATE_URGENT_MID;
            else if (p->bitrate_urgent_cnt >= BITRATE_URGENT_DEC_STEP3)
                p->bitrate_urgent_cnt -= BITRATE_URGENT_DEC_STEP3;
            else if (p->bitrate_urgent_cnt < BITRATE_URGENT_DEC_STEP3)
                p->bitrate_urgent_cnt = 0;
        } else if ((unsigned int) numFrameBits < p->target * 2 / 3) {
            VLOG(NONE,"base qp - 2:%d, bu_cnt:%d,qp_max:%d", p->quant, p->bitrate_urgent_cnt,qp_max);
            p->quant -= 2;
            if (p->quant < ADJ_QP_MIN)
                p->quant = ADJ_QP_MIN;
            if (p->bitrate_urgent_cnt > BITRATE_URGENT_MID)
                p->bitrate_urgent_cnt = BITRATE_URGENT_MID;
            else if (p->bitrate_urgent_cnt >= BITRATE_URGENT_DEC_STEP2)
                p->bitrate_urgent_cnt -= BITRATE_URGENT_DEC_STEP2;
            else if (p->bitrate_urgent_cnt < BITRATE_URGENT_DEC_STEP2)
                p->bitrate_urgent_cnt = 0;
        } else if ((unsigned int) numFrameBits < p->target * 9 / 10) {
            VLOG(NONE,"base qp--:%d, bu_cnt:%d,qp_max:%d", p->quant, p->bitrate_urgent_cnt,qp_max);
            p->quant--;
            if (p->quant < ADJ_QP_MIN)
                p->quant = ADJ_QP_MIN;
            if (p->bitrate_urgent_cnt > BITRATE_URGENT_MID)
                p->bitrate_urgent_cnt = BITRATE_URGENT_MID;
            else if (p->bitrate_urgent_cnt >= BITRATE_URGENT_DEC_STEP1)
                p->bitrate_urgent_cnt -= BITRATE_URGENT_DEC_STEP1;
            else if (p->bitrate_urgent_cnt < BITRATE_URGENT_DEC_STEP1)
                p->bitrate_urgent_cnt = 0;
        } else if (!p->bitrate_urgent_mode) {
            VLOG(NONE,"base qp:%d, bu_cnt:%d,qp_max:%d", p->quant, p->bitrate_urgent_cnt,qp_max);
            if (p->bitrate_urgent_cnt > BITRATE_URGENT_DEC_STEP1)
                p->bitrate_urgent_cnt -= BITRATE_URGENT_DEC_STEP1;
        }

        if (p->bitrate_urgent_cnt >= BITRATE_URGENT_TH) {
            p->bitrate_urgent_mode = true;
            p->bitrate_urgent_cnt = BITRATE_URGENT_TH;
        } else if (p->bitrate_urgent_mode &&
            (p->bitrate_urgent_cnt < BITRATE_URGENT_MID)) {
            p->bitrate_urgent_mode = false;
        }
        if (p->quant >= qp_max - 5)
            p->max_qp_count++;
        else
            p->max_qp_count = 0;

        if ((uint32_t)(p->src.mb_width * p->src.mb_height) > 3 * p->qp_stic.i_count) {
            p->need_inc_me_weight = 0;
            p->inc_me_weight_count = 0;
            if (p->quant > 35)
                p->quant = 35;
        } else {
            if (p->max_qp_count > 30) {
                if ((unsigned int) numFrameBits > p->target * 21 / 20) {
                    p->inc_me_weight_count++;
                    if (p->inc_me_weight_count > 3) {
                        p->need_inc_me_weight = 1;
                        p->inc_me_weight_count = 0;
                    }
                } else if ((unsigned int) numFrameBits < p->target * 19 / 20) {
                    p->inc_me_weight_count--;
                    if (p->inc_me_weight_count < -3) {
                        p->need_inc_me_weight = 0;
                        p->inc_me_weight_count = 0;
                    }
                } else {
                    p->inc_me_weight_count = 0;
                }
            } else {
                p->need_inc_me_weight = 0;
                p->inc_me_weight_count = 0;
            }
        }

        VLOG(NONE,"max_qp_count :%d, need_inc_me_weight:%d, inc_me_weight_count:%d,quant:%d",
            p->max_qp_count,
            p->need_inc_me_weight,
            p->inc_me_weight_count,
            p->quant);

        if (IDR)
            p->pre_quant = p->quant;
        else if ((uint32_t)(p->src.mb_width * p->src.mb_height) < 3 * p->qp_stic.i_count)
            p->pre_quant = p->quant;

        updateRateControl(rateCtrl, IDR);
        if (rateCtrl->skip_next_frame == -1) { // skip current frame
            status = AMVENC_SKIPPED_PICTURE;
            *skip_num = rateCtrl->skip_next_frame;
        }
    }
    VLOG(NONE,"Qp:%d", p->quant);
    return status;
}

void calculate_fix_qp(gx_fast_enc_drv_t *p, GxFastEncRateControl *rateCtrl)
{
    int qp_max;
    p->quant = 28 - (int) sqrt((double)rateCtrl->bitsPerFrame / p->src.mb_width / p->src.mb_height) * 4
        + p->qp_stic.f_sad_count * 6 / p->src.pix_width / p->src.pix_height;
    if (p->quant < 8)
        p->quant = 8;
    if (rateCtrl->bitsPerFrame / p->src.mb_width / p->src.mb_height < 16)
        qp_max = 38;
    else
        qp_max = 32;
    if (p->quant > qp_max)
        p->quant = qp_max;

    //ALOGI(" EB calculate quant:%d  bits compression:%d sad factor * 6:%d", p->quant,
    //    (int) sqrt((double)rateCtrl->bitsPerFrame / p->src.mb_width / p->src.mb_height) * 4,
    //    p->qp_stic.f_sad_count * 6 / p->src.pix_width / p->src.pix_height);
}

AMVEnc_Status GxFastRCInitFrameQP(void *dev, void *rc, bool IDR, int bitrate, float frame_rate) {
    int cur_ratio = 0;
    bool incr = false;
    gx_fast_enc_drv_t *p = (gx_fast_enc_drv_t *) dev;
    GxFastEncRateControl *rateCtrl = (GxFastEncRateControl *) rc;
    int fps = (int) (rateCtrl->estimate_fps + 0.5);
    int frame_duration_ms = 0;
//    char prop[PROPERTY_VALUE_MAX];
    int media_custom = 0;
//#ifdef SUPPORT_STANDARD_PROP
//    if (property_get("vendor.media.encoder.bitrate.custom", prop, NULL) > 0) {
//#else
//    if (property_get("media.encoder.bitrate.custom", prop, NULL) > 0) {
//#endif
//        sscanf(prop, "%d", &media_custom);
//    }
    //add for cts test
    media_custom = 0;
    bitrate = bitrate / 100 * 109.5;

    if (rateCtrl->rcEnable == true) {
        /* frame layer rate control */
        if (rateCtrl->encoded_frames == 0) {
            p->quant = rateCtrl->Qc = rateCtrl->initQP;
            p->target = bitrate / frame_rate * IDR_SCALER_RATIO;
            rateCtrl->refresh = false;
        } else {
            if (rateCtrl->frame_rate != frame_rate || rateCtrl->bitRate != bitrate || rateCtrl->force_IDR) {
                rateCtrl->refresh = true;
                rateCtrl->bitRate = bitrate;
                rateCtrl->frame_rate = frame_rate;
                BitrateScale(rateCtrl);
                VLOG(NONE,"we got new config, frame_rate:%f, bitrate:%d, force_IDR:%d rateCtrl->frame_rate%d", frame_rate, bitrate, rateCtrl->force_IDR, rateCtrl->frame_rate);
            } else {
                rateCtrl->refresh = false;
            }

            if (rateCtrl->refresh) {
                rateCtrl->buffer_fullness = rateCtrl->bitRate / 2;
                rateCtrl->estimate_fps = rateCtrl->frame_rate;
                rateCtrl->bitsPerFrame = (int) (rateCtrl->bitRate / rateCtrl->estimate_fps);
                rateCtrl->target = rateCtrl->bitsPerFrame;
                if (p->cbr_hw)
                    calculate_fix_qp(p, rateCtrl);
                if (IDR == 1)
                    rateCtrl->target = rateCtrl->bitsPerFrame*IDR_SCALER_RATIO;
                VLOG(NONE,"new: 1 buffer_fullness:%lld frame_rate:%f estimate_fps:%f target=%d", rateCtrl->buffer_fullness, rateCtrl->frame_rate, rateCtrl->estimate_fps, rateCtrl->target);
            } else {
                unsigned int bitsPerFrame = 0;
                frame_duration_ms = 1000 / fps;
                if ((unsigned int)rateCtrl->timecode > (unsigned int) rateCtrl->last_timecode)
                    bitsPerFrame = (rateCtrl->bitsPerFrame * ((unsigned int) rateCtrl->timecode - (unsigned int) rateCtrl->last_timecode) / frame_duration_ms);
                else
                    bitsPerFrame = rateCtrl->bitsPerFrame;

                if (bitsPerFrame > (unsigned int)(rateCtrl->bitsPerFrame * 13 / 10))
                    bitsPerFrame = rateCtrl->bitsPerFrame * 13 / 10;
                else if (bitsPerFrame < (unsigned int)(rateCtrl->bitsPerFrame * 7 / 10))
                    bitsPerFrame = rateCtrl->bitsPerFrame * 7 / 10;

                if (rateCtrl->buffer_fullness >= bitsPerFrame)
                    rateCtrl->buffer_fullness -= bitsPerFrame;
                else
                    rateCtrl->buffer_fullness = 0;

                cur_ratio = (rateCtrl->buffer_fullness * 100) / rateCtrl->bitRate;
                VLOG(NONE,"new: 2 buffer_fullness:%lld cur_ratio:%d bitsPerFrame:%d time interval:%d frame_duration_ms:%d fps:%d, estimate_fps:%f",
                        rateCtrl->buffer_fullness,
                        cur_ratio,
                        bitsPerFrame,
                        ((unsigned int) rateCtrl->timecode - (unsigned int) rateCtrl->last_timecode),
                        frame_duration_ms,
                        fps,
                        rateCtrl->estimate_fps);

                if (p->cbr_hw) {
                    if (cur_ratio < 10) {
                        cur_ratio = 10;
                        rateCtrl->buffer_fullness = rateCtrl->bitRate * cur_ratio / 100;
                    }
                } else {
                    if (cur_ratio < 40) {
                        cur_ratio = 40;
                        rateCtrl->buffer_fullness = rateCtrl->bitRate * cur_ratio / 100;
                    }
                    if (cur_ratio >= 200 || p->max_qp_count > 15) {
                        rateCtrl->buffer_fullness = rateCtrl->bitRate / 2;
                        cur_ratio = 50;
                    }
                }
                if (cur_ratio >= 150)
                    cur_ratio = 150;

                if (cur_ratio >= 50) {
                    cur_ratio -= 50;
                    cur_ratio = cur_ratio * fps / 10; /* 10 frame recover buffer_fullness */
                    if (cur_ratio > 50)
                        cur_ratio = 50;
                } else {
                    incr = true;
                    cur_ratio = 50 - cur_ratio;
                }

                //if(cur_ratio > 30)
                //    cur_ratio /= 3;
                //else if(cur_ratio > 15)
                //    cur_ratio /= 2;

                if (incr)
                    cur_ratio = 100 + cur_ratio;
                else
                    cur_ratio = 100 - cur_ratio;

                rateCtrl->target = (rateCtrl->bitsPerFrame * cur_ratio) / 100;

                if (IDR) {
                    rateCtrl->target = rateCtrl->bitsPerFrame * IDR_SCALER_RATIO * cur_ratio / 100;
                    if (!p->cbr_hw) {
                        if (p->pre_quant == 0)
                            p->quant = 27;
                        else
                            p->quant = p->pre_quant;
                    }
                }
                VLOG(NONE,"new: target :%d, bitsPerFrame:%d, cur_ratio:%d, IDR:%d buffer_fullness:%ld ",
                        rateCtrl->target,
                        bitsPerFrame,
                        cur_ratio,
                        rateCtrl->force_IDR,
                        rateCtrl->buffer_fullness);
            }
            p->target = rateCtrl->target;
        }
    } else {
	if (media_custom > 0) {
            if (IDR) {
                p->quant = 26;
            } else {
                p->quant = 26;
            }
	} else {
            p->quant = rateCtrl->initQP;
	}
    }
    return AMVENC_SUCCESS;
}

AMVEnc_Status GxFastRCUpdateBuffer(void *dev, void *rc, int timecode, bool force_IDR) {
    gx_fast_enc_drv_t *p = (gx_fast_enc_drv_t *) dev;
    GxFastEncRateControl *rateCtrl = (GxFastEncRateControl *) rc;
    rateCtrl->last_timecode = rateCtrl->timecode;
    rateCtrl->timecode = timecode;
    rateCtrl->force_IDR = force_IDR;
    return AMVENC_SUCCESS;
}

void GxFastCleanupRateControlModule(void *rc) {
    GxFastEncRateControl *rateCtrl = (GxFastEncRateControl *) rc;
    free(rateCtrl);
    return;
}

void* GxFastInitRateControlModule(amvenc_initpara_t* init_para) {
    GxFastEncRateControl *rateCtrl = NULL;

    if (!init_para)
        return NULL;

    rateCtrl = (GxFastEncRateControl*) calloc(1, sizeof(GxFastEncRateControl));
    if (!rateCtrl)
        return NULL;

    memset(rateCtrl, 0, sizeof(GxFastEncRateControl));

    rateCtrl->rcEnable = init_para->rcEnable;
    rateCtrl->initQP = init_para->initQP;
    rateCtrl->frame_rate = (float) init_para->frame_rate;
    rateCtrl->bitRate = init_para->bitrate;
    rateCtrl->cpbSize = init_para->cpbSize;
    rateCtrl->BitrateScale = init_para->bitrate_scale;
    BitrateScale(rateCtrl);

    if (rateCtrl->rcEnable == true) {
        rateCtrl->bitsPerFrame = (int32) (rateCtrl->bitRate / rateCtrl->frame_rate);
        rateCtrl->skip_next_frame = 0; /* must be initialized */
        rateCtrl->encoded_frames = 0;

        /* Setting the bitrate and framerate */
        rateCtrl->Qc = rateCtrl->initQP;
        rateCtrl->last_pframe_bits = 0x7fffffff;

        rateCtrl->buffer_fullness = rateCtrl->bitRate / 2;
        rateCtrl->timecode = 0;
        rateCtrl->last_timecode = 0;
        rateCtrl->estimate_fps = (float) rateCtrl->frame_rate;

        rateCtrl->refresh = false;
        rateCtrl->force_IDR = false;
    }
    return (void*) rateCtrl;

CLEANUP_RC:
    GxFastCleanupRateControlModule((void*) rateCtrl);
    return NULL;
}

