#include <assert.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "serial.h"
#include "printer.h"
#include "error_private.h"
#include "constants.h"

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


int main()
{
    char* str = malloc(100 * sizeof(char));
    

    int n = snprintf(str, 100, "%d %s     %.2f %.2f %.2f\n", 123, "teste", 2.00, 2.00, 4.00);


    escpos_printer *printer;
    printer = escpos_printer_network("192.168.1.100", 9100);
    escpos_printer_text(printer, "-- LISTA DE PRODUTOS --\n", 4, 8, 0);
    escpos_printer_text(printer, "------------------------\n", 4, 8, 0);
    escpos_printer_text(printer, "Nao tem valor fiscal\nAguarde a Nota Fiscal\n", 1, 1, 0);
    escpos_printer_text(printer, "------------------------\n", 4, 8, 0);
    escpos_printer_text(printer, "#   Desc      Qtd VlUnt.Subtotal\n", 1, 1, 0);
    escpos_printer_text(printer, "------------------------\n", 4, 8, 0);
    escpos_printer_text(printer, str, 1, 1, 0);
    // escpos_printer_raw(printer, "TESTE", 6);
    escpos_printer_feed(printer, 7);
    escpos_printer_cut(printer, 3);
    // escpos_printer_destroy(printer);


}

// "
// ------------------------------------------
// Não tem valor fiscal,aguarde a Nota Fiscal
// ------------------------------------------
// Codigo Descrição
// Qtd    UND             Vl. Unt.   Subtotal
// ------------------------------------------
// "