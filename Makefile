client: client.c
	gcc client.c -o client -lncurses

server: server.c
	gcc server.c -o server -lncurses

log: client.log
	@cat client.log

clean:
	rm -f client client.log server
