#include "subscriptor.h"
#include "comun.h"
#include "edsu_comun.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <pthread.h>

int puerto;

static void (*notifi)(const char*, const char*);

void* bucleAccept(void* socket);

int alta_subscripcion_tema(const char *tema) {
	return enviarMensaje(ALTA, tema, puerto);
}

int baja_subscripcion_tema(const char *tema) {
	return enviarMensaje(BAJA, tema, puerto);
}

int inicio_subscriptor(void (*notif_evento)(const char *, const char *),
		void (*alta_tema)(const char *), void (*baja_tema)(const char *)) {
	notifi = notif_evento;

	int s;
	struct sockaddr_in dir;
	int opcion = 1;

	if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("error creando socket");
		return 1;
	}

	/* Para reutilizar puerto inmediatamente
	 */
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opcion, sizeof(opcion)) < 0) {
		perror("error en setsockopt");
		return 1;
	}

	dir.sin_addr.s_addr = INADDR_ANY;
	dir.sin_port = 0;
	dir.sin_family = PF_INET;
	if (bind(s, (struct sockaddr *) &dir, sizeof(dir)) < 0) {
		perror("error en bind");
		close(s);
		return 1;
	}

	if (listen(s, MAX_LISTEN) < 0) {
		perror("error en listen");
		close(s);
		return 1;
	}
	pthread_t threadId;

	struct sockaddr_in aux;
	int tam = sizeof(aux);
	getsockname(s, (void*) &aux, (socklen_t *) &tam);

	puerto = ntohs(aux.sin_port);
	pthread_create(&threadId, NULL, bucleAccept, &s);

	return 0;
}

void* bucleAccept(void* socket) {

	int * socketNum = socket;
	int s_conec, leido;
	unsigned int tam_dir;
	struct sockaddr_in dir_cliente;
	char buf[TAM];
	char * mensaje = NULL;
	int mensajeLong = 0;
	while (1) {
		tam_dir = sizeof(dir_cliente);
		if ((s_conec = accept(*socketNum, (struct sockaddr *) &dir_cliente,
				&tam_dir)) < 0) {
			perror("error en accept");
			close(*socketNum);
			pthread_exit(NULL );

			return NULL ;
		}

		while ((leido = read(s_conec, buf, TAM)) > 0) {

			mensaje = realloc(mensaje, mensajeLong + leido * sizeof(char));

			memcpy((void*) mensaje, (void*) buf, leido);

		}

		notif notificacion = unMarshallMsg(mensaje);
		(*notifi)(notificacion->tema, notificacion->mensaje);

		if (leido < 0) {
			perror("error en read");
			close(*socketNum);
			close(s_conec);
			pthread_exit(NULL );
			return NULL ;
		}
		close(s_conec);
	}

	close(*socketNum);
	pthread_exit(NULL );
	return NULL ;
}

int fin_subscriptor() {
	return 0;
}

