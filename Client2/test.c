#include <stdio.h>
#include <string.h>
int main(){
    char url[]="kartik/something";
    char* username=strtok(url,"/");
    if(username!=NULL){
        printf("%s\n",username);
    }return 0;
}