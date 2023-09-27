all: udp tcp

tcp: tcp_client tcp_server

tcp_server:
	gcc tcp_server.c -o svr
	
tcp_client:
	gcc tcp_client.c -DPLAYER1 -o clt1; gcc tcp_client.c -DPLAYER2 -o clt2

udp: udp_client udp_server

udp_server:
	gcc udp_server.c -o svr
	
udp_client:
	gcc udp_client.c -DPLAYER1 -o clt1; gcc udp_client.c -DPLAYER2 -o clt2

clean:
	rm svr clt1 clt2