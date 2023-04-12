FROM ubuntu:latest

# ATUALIZANDO REPOSITORIOS
RUN apt update && apt upgrade -y


# INSTALANDO O QUE É NECESSÁRIO NESSA MÁQUINA LINUX
RUN apt install -y \
    nano \
    build-essential \
    manpages-dev \
    libmysqlclient-dev

RUN mkdir aulac

WORKDIR /aulac

COPY principal.c /aulac/
RUN gcc -o crud principal.c `mysql_config --cflags --libs`

ENTRYPOINT ["tail", "-f", "/dev/null"]