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

int escpos_printer_raw(escpos_printer *printer, const char *message, const int len)
{
    assert(printer != NULL);

    int total = sizeof(message);
    int sent = 0;
    int bytes = 0;

    if (printer->config.is_network_printer) {
        // Network printer logic.
        // Make sure send() sends all data
        bytes = send(printer->sockfd, message, total, 0);
        // while (sent < total) {
        //     bytes = send(printer->sockfd, message, total, 0);
        //     printf("%d", sent);
        //     if (bytes == -1) {
        //         // last_error = ESCPOS_ERROR_SEND_FAILED;
        //         break;
        //     } else {
        //         sent += bytes;
        //     }
        // }
    } else {
        // Serial printer logic.
        sent = write(printer->sockfd, message, len);
        if (sent != len) {
            // last_error = ESCPOS_ERROR_SEND_FAILED;
        }
        tcdrain(printer->sockfd);
    }

    return !(sent == total);
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

void set_bit(unsigned char *byte, const int i, const int bit)
{
    assert(byte != NULL);
    assert(i >= 0 && i < 8);

    if (bit > 0) {
        *byte |= 1 << i;
    } else {
        *byte &= ~(1 << i);
    }
}

// Calculates the padding required so that the size fits in 32 bits
void calculate_padding(const int size, int *padding_l, int *padding_r)
{
    assert(padding_l != NULL);
    assert(padding_r != NULL);

    if (size % 32 == 0) {
        *padding_l = 0;
        *padding_r = 0;
    } else {
        int padding = 32 - (size % 32);
        *padding_l = padding / 2;
        *padding_r = padding / 2 + (padding % 2 == 0 ? 0 : 1);
    }
}

void convert_image_to_bits(unsigned char *pixel_bits,
                           const unsigned char *image_data,
                           const int w,
                           const int h,
                           int *bitmap_w,
                           int *bitmap_h)
{
    assert(pixel_bits != NULL);
    assert(image_data != NULL);
    assert(bitmap_w != NULL);
    assert(bitmap_h != NULL);

    int padding_l, padding_r, padding_t, padding_b;
    calculate_padding(w, &padding_l, &padding_r);
    calculate_padding(h, &padding_t, &padding_b);

    int padded_w = w + padding_l + padding_r;

    // We only need to add the padding to the bottom for height.
    // This is because when printing long images, only the last
    // chunk will have the irregular height.
    padding_b += padding_t;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < padded_w; x++) {
            int pi = (y * padded_w) + x;
            int curr_byte = pi / 8;
            unsigned char pixel = image_data[(y * w) + x];
            int bit = y < h ? pixel < 128 : 0;
            set_bit(&pixel_bits[curr_byte], 7 - (pi % 8), bit);
        }
    }

    for (int y = 0; y < padding_b; y++) {
        for (int x = 0; x < padded_w; x++) {
            int pi = (h * padded_w) + (y * padded_w) + x;
            int curr_byte = pi / 8;
            set_bit(&pixel_bits[curr_byte], 7 - (pi % 8), 0);
        }
    }

    // Outputs the bitmap width and height after padding
    *bitmap_w = w + padding_l + padding_r;
    *bitmap_h = h + padding_b;
}

int escpos_printer_print(escpos_printer *printer,
                         const unsigned char *pixel_bits,
                         const int w,
                         const int h)
{
    assert(printer != NULL);
    assert(pixel_bits != NULL);
    assert(w > 0 && w <= printer->config.max_width);
    assert(h > 0 && h <= printer->config.chunk_height);
    assert(w % 32 == 0);
    assert(h % 32 == 0);

    int result = escpos_printer_raw(printer, ESCPOS_CMD_PRINT_RASTER_BIT_IMAGE, 4);

    char buffer[4];
    buffer[0] = (((w / 8) >> 0) & 0xFF);
    buffer[1] = (((w / 8) >> 8) & 0xFF);
    buffer[2] = ((h >> 0) & 0xFF);
    buffer[3] = ((h >> 8) & 0xFF);
    result = escpos_printer_raw(printer, buffer, 4);
    result = escpos_printer_raw(printer, (char *)pixel_bits, (w / 8) * h);

    if (result != 0) {
        // last_error = ESCPOS_ERROR_IMAGE_PRINT_FAILED;
    }

    return result;
}

int escpos_printer_image(escpos_printer *printer,
                         const unsigned char * const image_data,
                         const int width,
                         const int height)
{
    assert(printer != NULL);
    assert(image_data != NULL);
    assert(width > 0 && width <= printer->config.max_width);
    assert(height > 0);

    int result = 0;

    if (image_data != NULL) {
        int byte_width = printer->config.max_width / 8;

        int padding_t, padding_b;
        calculate_padding(height, &padding_t, &padding_b);
        int print_height = height + padding_t + padding_b;

        unsigned char pixel_bits[byte_width * print_height];

        int c = 0;
        int chunks = 0;
        if (height <= printer->config.chunk_height) {
            chunks = 1;
        } else {
            chunks = (height / printer->config.chunk_height) + (height % printer->config.chunk_height ? 1 : 0);
        }

        while (c < chunks) {
            // Because the printer's image buffer has a limited memory,
            // if the image's height exceeds config.chunk_height pixels,
            // it is printed in chunks of x * config.chunk_height pixels.
            int chunk_height = (c + 1) * printer->config.chunk_height <= height ?
                printer->config.chunk_height :
                height - (c * printer->config.chunk_height);

            int bitmap_w, bitmap_h;
            convert_image_to_bits(
                pixel_bits,
                image_data + (c * printer->config.chunk_height * width),
                width,
                chunk_height,
                &bitmap_w,
                &bitmap_h);

            result = escpos_printer_print(printer, pixel_bits, bitmap_w, bitmap_h);
            if (result != 0) {
                break;
            }

            c += 1;
        }
    }

    return result;
}

int escpos_printer_text(escpos_printer *printer, char *texto, int tamanho_fonte, int estilo, int justificativa) {
    // Monta o comando de formatação do texto com os parâmetros fornecidos
    char comando[100];
    snprintf(comando, sizeof(comando), "\x1B\x21%c", (tamanho_fonte - 1) * 16 + estilo);

    // Envia o comando para a impressora
    // escpos_printer_raw(printer, comando, sizeof(comando));
    
    // Monta o comando de justificativa do texto com o parâmetro fornecido
    // snprintf(comando, sizeof(comando), "\x1B\x61%c", justificativa);
    // return 1;
    // Envia o comando para a impressora
    escpos_printer_raw(printer, "Teste aaaaaa", sizeof(comando));
 
    
    // Envia o texto para a impressora
    // int bytes_enviados =  escpos_printer_raw(printer, "comando", sizeof(comando));
    
    
    // Retorna o número de bytes enviados para a impressora
    return 1;
}


int main()
{
    escpos_printer *printer;
    printer = escpos_printer_network("192.168.0.100", 9100);
    escpos_printer_text(printer, "Hello, world!", 3, 128, 0);
    // escpos_printer_raw(printer, "TESTE", 6);
    // escpos_printer_feed(printer, 5);
    // escpos_printer_cut(printer, 3);
    // escpos_printer_destroy(printer);


}