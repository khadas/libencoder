#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
unsigned int trans(char * buffer)
{
    unsigned int temp = 0;
	int i = 0;
	for (i = 0;i < 8; i++) {
		temp = temp << 4;
		if (buffer[i ] >= '0' && buffer[i] <= '9') {
            temp += buffer[i] - 0x30;
        }
        else if(buffer[i] >='a'&&buffer[i]<='f')
        {
            temp += buffer[i] - 0x61 + 0x0a;
        }
        else if(buffer[i] >= 'A' && buffer[i] <= 'F')
        {
            temp += buffer[i] - 0x41 + 0x0a;
        }
        else
        {
            printf("get wrong value:%d %02x ,file: %s ,line: %d\n", i, buffer[i],__FILE__,__LINE__);
        }
    }

    return temp;
}

int main(int argc,const char *argv[])
{
    FILE *fp;
    FILE *fid;
    size_t len = 0;
    char *str = NULL;
    ssize_t read;
	if (argc != 3) {
        fprintf(stderr,"usage: %s <src> <dst>\n",argv[0]);
        exit(1);
    }

    if ((fp = fopen(argv[1],"r")) == NULL) {
		perror("fopen()");
        exit(1);
    }

    fid = fopen(argv[2],"wb");
    if (fid == NULL)
    {
        printf("write file error!\n");
        exit(1);
    }
    while ((read = getline(&str,&len,fp)) != -1)
    {
        str[read-1]='\0';
        //fprintf(stdout,"%s\n",str);
        for ( int i=0; i< strlen(str); i+=8 ) {
            unsigned int tmp = trans(&str[i]);
            //printf("src: %08x \n",tmp);
            fwrite(&tmp,sizeof(unsigned char),4,fid);
        }
    }

    fclose(fid);
    fclose(fp);
    exit(0);
}
