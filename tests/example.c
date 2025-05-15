//Printa divisores de um número entrado

#include<stdio.h>
#include<stdlib.h>

int main(){
    int num;
    do{
        printf("Insira um número diferente de 0: ");
        scanf("%d", &num);
        if (num == 0){
            printf("\nDigite um número diferente de 0: ");
        }
    } while (num == 0);
    if (num < 0){
        num = -num;
    }

    for (int i = 1; i < num; i++){
        if (num % i == 0){
            printf("%d ", i);
        }
    }
}