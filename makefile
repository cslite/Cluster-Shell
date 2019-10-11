exe: clustershell_client.c clustershell_server.c
	gcc clustershell_server.c -o css
	gcc clustershell_client.c -o csc
