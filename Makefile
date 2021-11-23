all:
	g++ echo-client.cpp -o echo-client
	g++ echo-server.cpp -o echo-server

clean:
	rm -f echo-client echo-server
