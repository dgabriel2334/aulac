init:
	docker-compose up -d --build
down:
	@docker-compose down -v
build:
	docker exec -it aulac gcc -o crud principal.c `mysql_config --cflags --libs`
	docker exec -it aulac ./crud
.PHONY: clean test code-sniff init
