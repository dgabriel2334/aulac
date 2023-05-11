init:
	docker-compose up -d --build
down:
	@docker-compose down -v
build:
	docker exec -it aulac gcc -o crud principal.c `mysql_config --cflags --libs`
	docker exec -it aulac ./crud
build pdv:
	@docker exec -it aulac gcc -o pdv pdv.c `mysql_config --cflags --libs`
	clear
	docker exec -it aulac ./pdv
.PHONY: clean test code-sniff init
