#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

int main(){
	unlink("RESP_PIPE_12826");
	if(mkfifo("RESP_PIPE_12826", 0600) < 0) {
		printf("ERROR cannot create the response pipe\n");
		exit(1);
	}
	int fd_read=open("REQ_PIPE_12826",O_RDONLY);
	if(fd_read<0){
		printf("ERROR cannot open the request pipe\n");
		exit(1);
	}
	int fd_write=open("RESP_PIPE_12826",O_WRONLY);
	if(fd_write<0){
		printf("ERROR cannot open the response pipe\n");
		exit(1);
	}
	int aux=strlen("CONNECT");
	write(fd_write,&aux,1);
	write(fd_write,"CONNECT",strlen("CONNECT"));
	printf("SUCCESS");

	char buf[100];
	int n=strlen(buf),shm_fd=0,nr=0,size=0;
    char* data = NULL;
	char* file_data = NULL;
	while(1)
	{	
		read(fd_read,&n,1);
		read(fd_read,buf,n);
		if(strncmp("PING",buf,4)==0)
		{
			aux=strlen("PING");
			write(fd_write,&aux,1);
			write(fd_write,"PING",aux);
			aux=strlen("PONG");
			write(fd_write,&aux,1);
			write(fd_write,"PONG",aux);
			aux=12826;
			write(fd_write,&aux,sizeof(int));
		}
		if(strncmp("CREATE_SHM",buf,10)==0)
		{
			read(fd_read,&nr,sizeof(nr));
			shm_fd=shm_open("/07IfoU",O_CREAT | O_RDWR,0664);
			ftruncate(shm_fd,nr);
			if(shm_fd<0){
				aux=strlen("CREATE_SHM");
				write(fd_write,&aux,1);
				write(fd_write,"CREATE_SHM",aux);
				aux=strlen("ERROR");
				write(fd_write,&aux,1);
				write(fd_write,"ERROR",aux);
				exit(0);
			}
			aux=strlen("CREATE_SHM");
			write(fd_write,&aux,1);
			write(fd_write,"CREATE_SHM",aux);
			aux=strlen("SUCCESS");
			write(fd_write,&aux,1);
			write(fd_write,"SUCCESS",aux);
		}
		if(strncmp("WRITE_TO_SHM",buf,12)==0)
		{
			unsigned int offset=0;
			char value[5];
			read(fd_read,&offset,sizeof(offset));
			read(fd_read,&value,sizeof(value));
			data= (char*)mmap(0,nr, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
			if(data == MAP_FAILED || offset<0 || offset+sizeof(value)>=nr){
				aux=strlen("WRITE_TO_SHM");
				write(fd_write,&aux,1);
				write(fd_write,"WRITE_TO_SHM",aux);
				aux=strlen("ERROR");
				write(fd_write,&aux,1);
				write(fd_write,"ERROR",aux);
				exit(0);
			}
			value[5]='\0';
			strcpy(data+offset,value);
			aux=strlen("WRITE_TO_SHM");
			write(fd_write,&aux,1);
			write(fd_write,"WRITE_TO_SHM",aux);
			aux=strlen("SUCCESS");
			write(fd_write,&aux,1);
			write(fd_write,"SUCCESS",aux);
		}
		if(strncmp("MAP_FILE",buf,8)==0){
			read(fd_read,&aux,1);
			char file[aux];
			read(fd_read,&file,aux);
			file[aux]='\0';
			int fd = open(file,O_RDONLY);
			if(fd<0){
				aux=strlen("MAP_FILE");
				write(fd_write,&aux,1);
				write(fd_write,"MAP_FILE",aux);
				aux=strlen("ERROR");
				write(fd_write,&aux,1);
				write(fd_write,"ERROR",aux);
				exit(0);
			}
			size=lseek(fd,0,SEEK_END);
			lseek(fd,0,SEEK_SET);
			file_data=(char*)mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
			if(file_data == MAP_FAILED){
				aux=strlen("MAP_FILE");
				write(fd_write,&aux,1);
				write(fd_write,"MAP_FILE",aux);
				aux=strlen("ERROR");
				write(fd_write,&aux,1);
				write(fd_write,"ERROR",aux);
				exit(0);
			}
			aux=strlen("MAP_FILE");
			write(fd_write,&aux,1);
			write(fd_write,"MAP_FILE",aux);
			aux=strlen("SUCCESS");
			write(fd_write,&aux,1);
			write(fd_write,"SUCCESS",aux);	
		}
		if(strncmp("READ_FROM_FILE_OFFSET",buf,21)==0){	
			unsigned int offset=0,no_of_bytes=0;
			read(fd_read,&offset,sizeof(offset));
			read(fd_read,&no_of_bytes,sizeof(no_of_bytes));
			if(offset+no_of_bytes<=size){
				char read_data[no_of_bytes];
				for(int i=0;i<no_of_bytes;i++){
					read_data[i]=file_data[offset+i];
				}
				read_data[no_of_bytes]='\0';
				data = (char*)mmap(0,nr, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
				strcpy(data,read_data);
				data[no_of_bytes]='\0';
				aux=strlen("READ_FROM_FILE_OFFSET");
				write(fd_write,&aux,1);
				write(fd_write,"READ_FROM_FILE_OFFSET",aux);
				aux=strlen("SUCCESS");
				write(fd_write,&aux,1);
				write(fd_write,"SUCCESS",aux);
			}
			else{
				aux=strlen("READ_FROM_FILE_OFFSET");
				write(fd_write,&aux,1);
				write(fd_write,"READ_FROM_FILE_OFFSET",aux);
				aux=strlen("ERROR");
				write(fd_write,&aux,1);
				write(fd_write,"ERROR",aux);
			}
		}
		if(strncmp("READ_FROM_FILE_SECTION",buf,22)==0){
			unsigned int section_no=0,offset=0,no_of_bytes=0;
			read(fd_read,&section_no,sizeof(section_no));
			read(fd_read,&offset,sizeof(offset));
			read(fd_read,&no_of_bytes,sizeof(no_of_bytes));
			
			unsigned short heather_size=*((unsigned short*)(&file_data[size-4]));
			int start=size-heather_size;
			int no_sections=file_data[start+2];
			if(section_no>no_sections){
				aux=strlen("READ_FROM_FILE_SECTION");
				write(fd_write,&aux,1);
				write(fd_write,"READ_FROM_FILE_SECTION",aux);
				aux=strlen("ERROR");
				write(fd_write,&aux,1);
				write(fd_write,"ERROR",aux);
			}
			else{
				int top=start+3+21*section_no-8;
				int offset_s=*((unsigned int*)(&file_data[top]));
				char read_data[no_of_bytes];
				for(int i=0;i<no_of_bytes;i++){
					read_data[i]=file_data[offset+offset_s+i];
				}
				read_data[no_of_bytes]='\0';
				data = (char*)mmap(0,nr, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
				strcpy(data,read_data);
				data[no_of_bytes]='\0';
				aux=strlen("READ_FROM_FILE_SECTION");
				write(fd_write,&aux,1);
				write(fd_write,"READ_FROM_FILE_SECTION",aux);
				aux=strlen("SUCCESS");
				write(fd_write,&aux,1);
				write(fd_write,"SUCCESS",aux);
			}
		}
		if(strncmp("READ_FROM_LOGICAL_SPACE_OFFSET",buf,30)==0){	
			unsigned int offset=0,no_of_bytes=0;
			read(fd_read,&offset,sizeof(offset));
			read(fd_read,&no_of_bytes,sizeof(no_of_bytes));
			unsigned short heather_size=*((unsigned short*)(&file_data[size-4]));
			unsigned int offset_s,size_s;
			int start=size-heather_size;
			int no_sections=file_data[start+2];
			int multiple=3072;
			start+=3;
			int new_offset=0;
			for(int i=0;i<no_sections;i++){
				start+=21;
				offset_s=*((unsigned int*)(&file_data[start+13]));
				size_s=*((unsigned int*)(&file_data[start+17]));
				if(offset<multiple && new_offset==0){
					new_offset=offset;
				}
				offset-=multiple*(size_s/multiple+1);
			}
			if(new_offset==0){
				aux=strlen("READ_FROM_LOGICAL_SPACE_OFFSET");
				write(fd_write,&aux,1);
				write(fd_write,"READ_FROM_LOGICAL_SPACE_OFFSET",aux);
				aux=strlen("ERROR");
				write(fd_write,&aux,1);
				write(fd_write,"ERROR",aux);
			}
			else{
				
				char read_data[no_of_bytes];
				for(int i=0;i<no_of_bytes;i++){
					read_data[i]=file_data[new_offset+offset_s+i];
				}
				read_data[no_of_bytes]='\0';
				data = (char*)mmap(0,nr, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
				strcpy(data,read_data);
				data[no_of_bytes]='\0';
				aux=strlen("READ_FROM_LOGICAL_SPACE_OFFSET");
				write(fd_write,&aux,1);
				write(fd_write,"READ_FROM_LOGICAL_SPACE_OFFSET",aux);
				aux=strlen("SUCCESS");
				write(fd_write,&aux,1);
				write(fd_write,"SUCCESS",aux);
			}
			
		}
		if(strncmp("EXIT",buf,4)==0){	
			exit(0);
		}
	}
}
