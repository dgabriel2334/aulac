#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>
#include <stdlib.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#include "serial.h"
#include "printer.h"
#include "error_private.h"
#include "constants.h"

#define HOST "db"
#define USER "root"
#define PASS "123456"
#define DB "pdv"

MYSQL *conn;

typedef struct produto {
    int codigo;
    char nome[50];
    float preco;
} Produto;


escpos_printer *escpos_printer_network(const char * const addr, const short port)
{
    assert(addr != NULL);

    int sockfd;
    escpos_printer *printer = NULL;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        // last_error = ESCPOS_ERROR_SOCK;
    } else {
        struct sockaddr_in dest;
        dest.sin_family = AF_INET;
        dest.sin_port = htons(port);

        if (inet_pton(AF_INET, addr, &dest.sin_addr.s_addr) == 0) {
            // last_error = ESCPOS_ERROR_INVALID_ADDR;
        } else if (connect(sockfd, (struct sockaddr*)&dest, sizeof(dest)) != 0) {
            // last_error = ESCPOS_ERROR_CONNECTION_FAILED;
        } else {
            printer = (escpos_printer *)malloc(sizeof(escpos_printer));
            printer->sockfd = sockfd;
            printer->config.max_width = ESCPOS_MAX_DOT_WIDTH;
            printer->config.chunk_height = ESCPOS_CHUNK_DOT_HEIGHT;
            printer->config.is_network_printer = 1;
        }
    }

    return printer;
}

void escpos_printer_destroy(escpos_printer *printer)
{
    assert(printer != NULL);

    close(printer->sockfd);
    free(printer);
}

int escpos_printer_cut(escpos_printer *printer, const int lines)
{
    char buffer[4];
    strncpy(buffer, ESCPOS_CMD_CUT, 3);
    buffer[3] = lines;
    return escpos_printer_raw(printer, buffer, sizeof(buffer));
}

int escpos_printer_feed(escpos_printer *printer, const int lines)
{
    assert(lines > 0 && lines < 256);

    char buffer[3];
    strncpy(buffer, ESCPOS_CMD_FEED, 2);
    buffer[2] = lines;
    return escpos_printer_raw(printer, buffer, sizeof(buffer));
}

int escpos_printer_raw(escpos_printer *printer, const char *message, const int len)
{
    assert(printer != NULL);

    int total = len;
    int sent = 0;
    int bytes = 0;

    if (printer->config.is_network_printer) {
        // Network printer logic.
        // Make sure send() sends all data
        while (sent < total) {
            bytes = send(printer->sockfd, message + sent, total - sent, 0);
            if (bytes == -1) {
                // last_error = ESCPOS_ERROR_SEND_FAILED;
                break;
            } else {
                sent += bytes;
            }
        }
    }  else {
        // Serial printer logic.
        sent = write(printer->sockfd, message, len);
        if (sent != len) {
            // last_error = ESCPOS_ERROR_SEND_FAILED;
        }
        tcdrain(printer->sockfd);
    }

    return !(sent == total);
}


int escpos_printer_text(escpos_printer *printer, char *texto, int tamanho_fonte, int estilo, int justificativa) {
    // Monta o comando de formatação do texto com os parâmetros fornecidos
    char comando1[5];
    snprintf(comando1, sizeof(comando1), "\x1B\x21%c", (tamanho_fonte - 1) * 16 + estilo);

    // Envia o comando de formatação para a impressora
    escpos_printer_raw(printer, comando1, sizeof(comando1));
    
    // Monta o comando de justificativa do texto com o parâmetro fornecido
    char comando2[4];
    snprintf(comando2, sizeof(comando2), "\x1B\x61%c", justificativa);

    // Envia o comando de justificativa para a impressora
    escpos_printer_raw(printer, comando2, sizeof(comando2));

    // Envia o texto para a impressora
    escpos_printer_raw(printer, texto, strlen(texto));

    // Retorna o número de bytes enviados para a impressora
    return strlen(texto);
}

void cadastrar_produto() {
    Produto p;

    MYSQL *conexao = mysql_init(NULL);
    //Conexão com o banco de dados
    if (!mysql_real_connect(conexao, "db", "root", "123456", "pdv", 0, NULL, 0)) {
        printf("Erro: %s\n", mysql_error(conexao));
    }

    printf("\n--- Cadastro de Produto ---\n");

    printf("Código: ");
    scanf("%d", &p.codigo);

    printf("Nome: ");
    scanf(" %[^\n]s", p.nome);

    printf("Preço: R$ ");
    scanf("%f", &p.preco);
    char query[200] = "";
    sprintf(query, "INSERT INTO produtos (id, codigo, nome, preco) VALUES (NULL, %d, '%s', %.2f)", p.codigo, p.nome, p.preco);

    if (mysql_query(conexao, query)) {
        printf("Erro: %s\n", mysql_error(conexao));
    } else {
        printf("Produto inserido com sucesso!\n");
    }
}

void listar_produtos() {
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[100] = "";

    printf("\n--- Lista de Produtos ---\n");

    MYSQL *conexao = mysql_init(NULL);
    if (!mysql_real_connect(conexao, "db", "root", "123456", "pdv", 0, NULL, 0)) {
        printf("Erro: %s\n", mysql_error(conexao));
    }
    sprintf(query, "SELECT * FROM produtos");
    if (mysql_query(conexao, query)) {
        printf("Erro: %s\n", mysql_error(conexao));
    } else { 
        res = mysql_store_result(conexao);

        while ((row = mysql_fetch_row(res))) {
            printf("Código: %s | Nome: %s | Preço: R$ %s\n", row[1], row[2], row[3]);
        }
    }

    mysql_free_result(res);
}

void realizar_venda() {
    Produto produtos[100];
    int codigo, quantidade;
    float total = 0;
    int i = 0;

    MYSQL *conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, "db", "root", "123456", "pdv", 0, NULL, 0)) {
        printf("Erro: %s\n", mysql_error(conn));
    }

    printf("\n--- Realizar Venda ---\n");

    while (1) {
        printf("Digite o código do produto (0 para finalizar a venda): ");
        scanf("%d", &codigo);

        if (codigo == 0) {
            break;
        }

        char query[100];
        sprintf(query, "SELECT nome, preco, codigo FROM produtos WHERE codigo=%d", codigo);

        if (mysql_query(conn, query)) {
            fprintf(stderr, "%s\n", mysql_error(conn));
            continue;
        }

        MYSQL_RES *res = mysql_use_result(conn);
        MYSQL_ROW row = mysql_fetch_row(res);

        if (!row) {
            printf("Produto não encontrado!\n");
            mysql_free_result(res);
            continue;
        }

        produtos[i].codigo = atoi(row[3]);
        strcpy(produtos[i].nome, row[0]);
        produtos[i].preco = atof(row[1]);
        i++;
        printf("Nome: %s | Preço: R$ %s\n", row[0], row[1]);

        // printf("Digite a quantidade: ");
        // scanf("%d", &quantidade);
        quantidade = 1;

        float preco_unitario = atof(row[1]);
        total += preco_unitario * quantidade;

        mysql_free_result(res);
    }

    imprimir(produtos, i);

    printf("Total da venda: R$ %.2f\n", total);
}

char *pegar_data_hora() {
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);

    // Ajusta o fuso horário para o horário de Manaus (UTC-4)
    local_time->tm_hour -= 4;
    mktime(local_time);

    // Formata a data e hora de acordo com o formato desejado
    static char data_hora[20];
    strftime(data_hora, sizeof(data_hora), "%d/%m/%Y %H:%M", local_time);

    return data_hora;
}

int imprimir(Produto produtos[], int qtd)
{
    char* str = malloc(100 * sizeof(char));
    char* strTotal = malloc(100 * sizeof(char));
    float total = 0;
    escpos_printer *printer;
    printer = escpos_printer_network("192.168.1.100", 9100);
    escpos_printer_text(printer, "-- SUPER ESTACIO --\n", 4, 8, 1);
    escpos_printer_text(printer, "Razao Social: Estacio de SA\n", 1, 1, 0);
    escpos_printer_text(printer, "Av. Constantino Nery, 3693 - Chapada, Manaus - AM, 69025-315\n", 1, 1, 0);
    escpos_printer_text(printer, "CNPJ: 034.075.739/0001-84\n", 1, 1, 0);
    escpos_printer_text(printer, pegar_data_hora(), 1, 1, 0);
    escpos_printer_text(printer, "\n", 1, 1, 0);
    escpos_printer_text(printer, "------------------------\n", 4, 8, 0);
    escpos_printer_text(printer, "Nao tem valor fiscal\nAguarde a Nota Fiscal\n", 1, 1, 0);
    escpos_printer_text(printer, "------------------------\n", 4, 8, 0);
    escpos_printer_text(printer, "Cliente: Consumidor Final\n", 1, 1, 0);
    escpos_printer_text(printer, "------------------------\n", 4, 8, 0);
    escpos_printer_text(printer, "-- LISTA DE PRODUTOS --\n", 4, 8, 0);
    escpos_printer_text(printer, "------------------------\n", 4, 8, 0);
    escpos_printer_text(printer, "#   Desc      Qtd VlUnt.Subtotal\n", 1, 1, 0);
    escpos_printer_text(printer, "------------------------\n", 4, 8, 0);
    int j;
    for (j = 0; j < qtd; j++) {
        snprintf(str, 100, "%d   %s         %d %.2f   %.2f\n", j+1, produtos[j].nome, 1, produtos[j].preco, produtos[j].preco);
        escpos_printer_text(printer, str, 1, 1, 0);
        memset(str, 0, sizeof(str));
        total = total + produtos[j].preco;
        // printf("Produto %d:\n", j+1);
        // printf("Código: %d\n", produtos[j].codigo);
        // printf("Nome: %s\n", produtos[j].nome);
        // printf("Preço: R$ %.2f\n", produtos[j].preco);
        // printf("\n");
    }
    snprintf(strTotal, 100, "TOTAL: %.2f\n", total);
    escpos_printer_text(printer, "------------------------\n", 4, 8, 0);
    escpos_printer_text(printer, strTotal, 4, 8, 1);
    escpos_printer_text(printer, "------------------------\n", 4, 8, 0);
    escpos_printer_feed(printer, 7);
    escpos_printer_cut(printer, 3);
}

int main()
{
    // Conexão com o banco de dados
    MYSQL *conn = mysql_init(NULL);

    if (!mysql_real_connect(conn, HOST, USER, PASS, DB, 0, NULL, 0))
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    // Criação da tabela de produtos caso ainda não exista
    // create_table(conn);

    // Menu principal
    int opcao = 0;

    while (opcao != 4)
    {
        printf("\n=== Mini PDV ===\n");
        printf("1 - Cadastrar Produto\n");
        printf("2 - Listar Produtos\n");
        printf("3 - Realizar Venda\n");
        printf("4 - Sair\n");
        printf("Escolha uma opção: ");
        scanf("%d", &opcao);

        switch (opcao)
        {
            case 1:
                cadastrar_produto();
                break;
            case 2:
                listar_produtos();
                break;
            case 3:
                realizar_venda();
                break;
            case 4:
                printf("Saindo...\n");
                break;
            default:
                printf("Opção inválida!\n");
                break;
        }
    }

    // Fechando conexão com o banco de dados
    mysql_close(conn);

    return 0;
}
