#include <winsock2.h>
#include <iostream>
#include <stdio.h>
#include <conio.h>
#include <sstream>

using namespace std;
class transfer{
    public:
        SOCKET *socket;

        int readS(char *buffer,int len){
            int cnt,rc;
            cnt=len;
            while(cnt>0){
                rc=recv(*socket,buffer,cnt,0);
                if(rc<=0){
                    //puts("Recv call failed. Error");
                    return -1;
                }
                buffer+=rc;
                cnt-=rc;
            }
            return len;
        }
        int readN(char *buffer,int len, string& rez){
            int k=readS(buffer,len);
            if(k<0){
                //puts("Recv call failed. Error");
                return 0;
            }
            for(int i=0;i<len;i++)
                rez=rez+buffer[i];
            return 1;
        }
        int readTillSeparator(char *buffer, string& rez){
            int rc=1;
            while(rc!=0){

                rc=recv(*socket,buffer,1,0);
                if(rc<=0){
                    //puts("Recv call failed. Error");
                    return 0;
                }
                if(buffer[0]=='\n')
                    return 1;
                rez=rez+buffer[0];
            }
        }
        int sendMSG(char* buffer){
            int res=send(*socket,buffer,strlen(buffer),0);
            if(res<=0)
                return 0;
            return 1;
        }
};
