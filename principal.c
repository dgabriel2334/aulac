/**
-**************************FEITO POR:**********************-
-*              Gabriel Santos - Vulgo: @gabi             *-
-*            Modificado em: 06/04/2023 às 16:10          *-
-*    Com grandes poderes vem grandes responsabilidades   *-
-**********************************************************-
-*           Me contate no WhatsApp pelos números:        *-
-*             (11) 91092-1684 | (92) 98128-5787          *-
-**********************************************************-
-*                    Me contate por email:               *-
-*                 dariogabriel2334@gmail.com             *-
-**********************************************************-
-*             Faça uma doação voluntária via PIX         *-
-*                   Celular: (11) 91092-1684             *-
-*                      CPF: 007.284.902-93               *-
-**********************************************************-
*/

#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>
#include <stdlib.h>

//Definindo um usuário
struct usuario {
   int id;
   char nome[50];
};

void inserir() {
    char nome_usuario[100];

    printf("Digite o nome para o usuario (sem espacos): ");
    scanf("%s", nome_usuario);

    char query[200] = "";
    //Conexão com o banco de dados
    MYSQL *conexao = mysql_init(NULL);
    if (!mysql_real_connect(conexao, "db", "root", "123456", "crud", 0, NULL, 0)) {
        printf("Erro: %s\n", mysql_error(conexao));
    }

    //Montando o comando SQL
    sprintf(query, "INSERT INTO usuario VALUES (NULL, '%s')", nome_usuario);

    //Executa o query
    if (mysql_query(conexao, query) ) {

    printf("Erro: %s\n", mysql_error(conexao));

    } else
    printf("Usuário inserido com sucesso!\n");

    mysql_close(conexao);
    menu();
}

struct usuario* buscar(){

    char nome_usuario[50];
    int resultados;
    struct usuario *u;

    u = (struct usuario*) malloc(sizeof(struct usuario));

    printf("Digite o nome do usuario: ");
    scanf("%s", nome_usuario);

    MYSQL *conexao = mysql_init(NULL);
    if (!mysql_real_connect(conexao, "db", "root", "123456", "crud", 0, NULL, 0)) {
        printf("Erro: %s\n", mysql_error(conexao));
    }
        
    char query[100] = "";
    MYSQL_RES *resposta;
    MYSQL_ROW linhas;
    
    //Montando o comando SQL
    sprintf(query, "SELECT * FROM usuario WHERE nome LIKE '%%s%%'", nome_usuario);
    
    //Executa o query
    if (mysql_query(conexao, query) ) {
        printf("Erro: %s\n", mysql_error(conexao));
    } else {    

        resposta = mysql_store_result(conexao);
        resultados = mysql_num_rows(resposta);

        if (resultados < 1) {
            printf("\nO usuario: %s nao foi encontrado", nome_usuario);
        } else {
            //Recebe o resultado da query
            //Recebe uma linha do resultado da query
            
            printf("-------------------------------------\n");
            printf("|id|nome|\n");
            printf("-------------------------------------\n");
            while((linhas = mysql_fetch_row(resposta)))
            {
                u->id = atoi (linhas[0]);
                strcpy(u->nome, linhas[1]);
                printf("|%d|", u->id);
                printf("%s\n", u->nome);
            }
            printf("\nTOTAL DE RESULTADOS ENCONTRADOS %d", resultados);
        }
    }
    
    mysql_close(conexao);
    menu();
}

struct usuario* listar(){

    struct usuario *u;
    u = (struct usuario*) malloc(sizeof(struct usuario));

    MYSQL *conexao = mysql_init(NULL);
    if (!mysql_real_connect(conexao, "db", "root", "123456", "crud", 0, NULL, 0)) {
        printf("Erro: %s\n", mysql_error(conexao));
    }
        
    char query[100] = "";
    MYSQL_RES *resposta;
    MYSQL_ROW linhas;
    
    //Montando o comando SQL
    sprintf(query, "SELECT * FROM usuario");
    
    //Executa o query
    if (mysql_query(conexao, query) ) {
        printf("Erro: %s\n", mysql_error(conexao));
    } else {    
        
        //Recebe o resultado da query
        resposta = mysql_store_result(conexao);
        //Recebe uma linha do resultado da query
        
        printf("-------------------------------------\n");
        printf("|id|nome|\n");
        printf("-------------------------------------\n");
        while((linhas = mysql_fetch_row(resposta)))
        {
            u->id = atoi (linhas[0]);
            strcpy(u->nome, linhas[1]);
            printf("|%d|", u->id);
            printf("%s\n", u->nome);
        }
    }
    
    mysql_close(conexao);
    menu();
}

void alterar(){
    int id_usuario;
    char nome_usuario[50];
    char query[100] = "";

    struct usuario *usuario;
    usuario = (struct usuario*) malloc(sizeof(struct usuario));

    printf("Digite o id do usuario a ser editado: ");
    scanf("%d", &id_usuario);

    printf("Digite o novo nome do usuario: ");
    scanf("%s", nome_usuario);

    usuario->id = id_usuario;
    strcpy(usuario->nome, nome_usuario);

    //Conexão com o banco de dados
    MYSQL *conexao = mysql_init(NULL);
    if (!mysql_real_connect(conexao, "db", "root", "123456", "crud", 0, NULL, 0)) {
            printf("Erro: %s\n", mysql_error(conexao));
    }
        
    //Montando o comando SQL
    sprintf(query, "UPDATE usuario SET nome = '%s' WHERE id = %d", usuario->nome, usuario->id);
    
    //Executa o query
    if (mysql_query(conexao, query) ) {
        
        printf("Erro: %s\n", mysql_error(conexao));
        
    } else
        printf("Usuário alterado com sucesso!\n");
        
    mysql_close(conexao);
    menu();

}

void remover(){
    int id_usuario;
    char query[100] = "";

    printf("Digite o id do usuario a ser deletado: ");
    scanf("%d", &id_usuario);
    //Conexão com o banco de dados
    MYSQL *conexao = mysql_init(NULL);
    if (!mysql_real_connect(conexao, "db", "root", "123456", "crud", 0, NULL, 0)) {
        printf("Erro: %s\n", mysql_error(conexao));
    }
        
    //Montando o comando SQL
    sprintf(query, "DELETE FROM usuario WHERE id = %d", id_usuario);
    
    //Executa o query
    if (mysql_query(conexao, query) ) {
        
        printf("Erro: %s\n", mysql_error(conexao));
        
    } else
        printf("Usuário removido com sucesso!\n");
        
    mysql_close(conexao);
    menu();
}

int menu(){
    int i;
    int opcao;

    for (int i = 0; i < 2; i++)
    {
        printf("\n");
    }
    printf("***********************************************************\n");
    printf("###########################################################\n");
    printf("################### BEM VINDO AO CRUD #####################\n");
    printf("###########################################################\n");
    printf("***********************************************************\n");

    for (int i = 0; i < 3; i++)
    {
        printf("\n");
    }

    printf("1 - CADASTRAR USUARIO\n");
    printf("2 - LISTAR USUARIOS\n");
    printf("3 - ALTERAR USUARIO\n");
    printf("4 - DELETAR USUARIO\n");
    printf("5 - BUSCAR USUARIO\n");
    printf("0 - SAIR\n");
    printf("Selecione sua opção:\n");

    scanf("%d", &opcao);

    if (opcao == 0){
        return 1;
    }

    switch (opcao)
    {
        case 1:
            inserir();
            break;
        case 2:
            listar();
            break;
        case 3:
            alterar();
            break;
        case 4:
            remover();
            break;
        case 5:
            buscar();
            break;
        default:
            menu();
            break;
    }
    
    return 1;
    
}

int main() {
    menu();
    return 0;
}