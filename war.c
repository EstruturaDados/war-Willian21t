/*
  jogo.c
  Sistema simplificado de "missões" para jogadores (exercício em C)
  Requisitos aplicados: modularização, ponteiros, malloc/free, strcpy, srand(time(NULL)), etc.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Estrutura de território conforme especificado */
typedef struct {
    char nome[30];
    char cor[10];   /* cor do dono: ex "vermelha", "azul", "nenhuma" */
    int tropas;
} Territorio;

/* Protótipos das funções (modularização) */
void atribuirMissao(char* destino, char* missoes[], int totalMissoes);
int verificarMissao(char* missao, Territorio* mapa, int tamanho);
void exibirMissao(const char* missao); /* por valor para exibição */
void atacar(Territorio* atacante, Territorio* defensor);
void exibirMapa(Territorio* mapa, int tamanho);
void liberarMemoria(Territorio* mapa, int tamanho, char** missoesAlocadas, int totalPlayers);

/* --- Implementações --- */

/* Sorteia uma missão e copia para 'destino' usando strcpy.
   Nota: destino deve já apontar para memória alocada com tamanho suficiente. */
void atribuirMissao(char* destino, char* missoes[], int totalMissoes) {
    int idx = rand() % totalMissoes;
    strcpy(destino, missoes[idx]);
}

/* Verifica se a missão (string) foi cumprida no mapa.
   As missões usadas aqui seguem padrões conhecidos e a função faz um parsing simples.
   Retorna 1 se cumprida, 0 caso contrário.
   Observação: no nosso sistema, a missão pode vir no formato "COR|TEXTO_DA_MISSAO",
   onde antes do '|' temos a cor do jogador responsável pela missão. */
int verificarMissao(char* missao, Territorio* mapa, int tamanho) {
    if (missao == NULL) return 0;

    /* Separamos cor (até '|') e texto da missão */
    char copia[200];
    strncpy(copia, missao, sizeof(copia)-1);
    copia[sizeof(copia)-1] = '\0';

    char* sep = strchr(copia, '|');
    char corJogador[16];
    char textoMissao[160];

    if (sep) {
        *sep = '\0';
        strncpy(corJogador, copia, sizeof(corJogador)-1);
        corJogador[sizeof(corJogador)-1] = '\0';
        strncpy(textoMissao, sep+1, sizeof(textoMissao)-1);
        textoMissao[sizeof(textoMissao)-1] = '\0';
    } else {
        /* Se não houver cor embutida, tratamos toda string como texto */
        corJogador[0] = '\0';
        strncpy(textoMissao, copia, sizeof(textoMissao)-1);
        textoMissao[sizeof(textoMissao)-1] = '\0';
    }

    /* Verificações simples para alguns tipos de missão: */
    /* 1) "Conquistar 3 territorios seguidos" -> verifica se o jogador (corJogador) possui 3 territorios consecutivos */
    if (strstr(textoMissao, "Conquistar 3 territorios seguidos") != NULL) {
        if (strlen(corJogador) == 0) return 0; /* sem cor não faz sentido */
        int consecutivos = 0;
        for (int i = 0; i < tamanho; ++i) {
            if (strcmp(mapa[i].cor, corJogador) == 0) {
                consecutivos++;
                if (consecutivos >= 3) return 1;
            } else consecutivos = 0;
        }
        return 0;
    }

    /* 2) "Eliminar todas as tropas da cor X" -> detecta cor X na string (após "cor ") */
    if (strstr(textoMissao, "Eliminar todas as tropas da cor") != NULL) {
        /* vamos buscar uma palavra de cor no final da frase */
        char *p = strstr(textoMissao, "cor");
        if (!p) return 0;
        p += 3; /* pula "cor" */
        while (*p == ' ') p++;
        char corAlvo[16];
        sscanf(p, "%15s", corAlvo); /* pega a cor alvo */
        /* verifica se existe território ainda com tropas dessa cor */
        for (int i = 0; i < tamanho; ++i) {
            if (strcmp(mapa[i].cor, corAlvo) == 0 && mapa[i].tropas > 0) return 0;
        }
        return 1; /* se não encontrou tropas dessa cor, missão cumprida */
    }

    /* 3) "Possuir ao menos N tropas no total" -> captura N e soma tropas do jogador (pela cor embutida) */
    if (strstr(textoMissao, "Possuir ao menos") != NULL && strstr(textoMissao, "tropas") != NULL) {
        if (strlen(corJogador) == 0) return 0;
        int N = 0;
        /* tenta extrair número da frase */
        char *p = strstr(textoMissao, "Possuir ao menos");
        if (p) {
            /* procura um número */
            while (*p && (*p < '0' || *p > '9')) p++;
            if (*p) N = atoi(p);
        }
        if (N <= 0) return 0;
        int soma = 0;
        for (int i = 0; i < tamanho; ++i) {
            if (strcmp(mapa[i].cor, corJogador) == 0) soma += mapa[i].tropas;
        }
        return (soma >= N) ? 1 : 0;
    }

    /* Caso padrão: não reconheceu a missão -> retorna 0 (não cumprida) */
    return 0;
}

/* Exibe missão (passagem por valor) */
void exibirMissao(const char* missao) {
    if (missao == NULL) {
        printf("Nenhuma missão.\n");
        return;
    }
    /* Caso a missão esteja no formato COR|TEXTO, exibimos apenas o TEXTO para o jogador (interface intuitiva) */
    const char* sep = strchr(missao, '|');
    if (sep) {
        printf("%s\n", sep + 1);
    } else {
        printf("%s\n", missao);
    }
}

/* Função de ataque entre territórios.
   - Usa rand() para rolar 1..6 para atacante e defensor (simples).
   - Se atacante vence: defensor muda de cor para atacante e recebe metade das tropas do atacante.
   - Se defensor vence: atacante perde 1 tropa.
*/
void atacar(Territorio* atacante, Territorio* defensor) {
    if (!atacante || !defensor) return;

    if (strcmp(atacante->cor, defensor->cor) == 0) {
        printf("Ataque inválido: mesmo dono.\n");
        return;
    }
    if (atacante->tropas < 2) {
        printf("Ataque inválido: atacante precisa ter ao menos 2 tropas para atacar.\n");
        return;
    }

    int rollA = (rand() % 6) + 1;
    int rollD = (rand() % 6) + 1;
    printf("Rolagem: atacante %d x defensor %d\n", rollA, rollD);

    if (rollA > rollD) {
        /* atacante vence */
        int transfer = atacante->tropas / 2; /* metade das tropas transferidas */
        if (transfer < 1) transfer = 1;
        printf("Atacante vence! Transferindo %d tropas e mudando cor de %s para %s.\n",
               transfer, defensor->nome, atacante->cor);
        defensor->tropas = transfer;
        strcpy(defensor->cor, atacante->cor);
        atacante->tropas -= transfer;
        if (atacante->tropas < 0) atacante->tropas = 0;
    } else {
        /* defensor vence (ou empata) */
        atacante->tropas -= 1;
        if (atacante->tropas < 0) atacante->tropas = 0;
        printf("Defensor resiste! Atacante perde 1 tropa.\n");
    }
}

/* Exibe o mapa de territórios */
void exibirMapa(Territorio* mapa, int tamanho) {
    printf("Mapa atual:\n");
    for (int i = 0; i < tamanho; ++i) {
        printf(" %2d) %-12s | cor: %-8s | tropas: %2d\n", i, mapa[i].nome, mapa[i].cor, mapa[i].tropas);
    }
}

/* Libera memória alocada para missões e (no caso de ter alocado dinamicamente o mapa) libera mapa.
   Aqui mapa foi alocado em main, então liberamos apenas missões (strings) e o mapa. */
void liberarMemoria(Territorio* mapa, int tamanho, char** missoesAlocadas, int totalPlayers) {
    /* libera as missões alocadas para cada jogador */
    for (int i = 0; i < totalPlayers; ++i) {
        if (missoesAlocadas[i]) {
            free(missoesAlocadas[i]);
            missoesAlocadas[i] = NULL;
        }
    }
    /* libera o vetor de ponteiros de missões */
    if (missoesAlocadas) free(missoesAlocadas);

    /* libera mapa se alocado */
    if (mapa) free(mapa);
}

/* --- Programa principal (main) --- */
int main() {
    srand((unsigned int)time(NULL));

    /* Definições iniciais (simulação pequena) */
    const int totalPlayers = 2;
    const int tamanhoMapa = 8;

    /* Vetor de descrições de missões (pelo menos 5) */
    char* missoesPadrao[] = {
        "Conquistar 3 territorios seguidos",
        "Eliminar todas as tropas da cor vermelha",
        "Possuir ao menos 10 tropas no total",
        "Conquistar 2 territorios seguidos",
        "Eliminar todas as tropas da cor azul"
    };
    const int totalMissoesPadrao = sizeof(missoesPadrao) / sizeof(missoesPadrao[0]);

    /* Criamos um mapa simples (alocado dinamicamente conforme pedido) */
    Territorio* mapa = (Territorio*) calloc(tamanhoMapa, sizeof(Territorio));
    if (!mapa) {
        fprintf(stderr, "Erro: nao foi possivel alocar mapa.\n");
        return 1;
    }
    /* Inicializa territórios */
    const char* nomesInit[] = {"A1","A2","B1","B2","C1","C2","D1","D2"};
    const char* coresInit[] = {"vermelha","azul","vermelha","nenhuma","azul","vermelha","azul","nenhuma"};
    int tropasInit[] = {3, 2, 4, 1, 5, 2, 3, 1};

    for (int i = 0; i < tamanhoMapa; ++i) {
        strncpy(mapa[i].nome, nomesInit[i], sizeof(mapa[i].nome)-1);
        mapa[i].nome[sizeof(mapa[i].nome)-1] = '\0';
        strncpy(mapa[i].cor, coresInit[i], sizeof(mapa[i].cor)-1);
        mapa[i].cor[sizeof(mapa[i].cor)-1] = '\0';
        mapa[i].tropas = tropasInit[i];
    }

    /* Informações dos jogadores (nomes e cores fixas para a simulação) */
    char* nomesPlayers[totalPlayers];
    nomesPlayers[0] = "Jogador 1";
    nomesPlayers[1] = "Jogador 2";
    char* coresPlayers[totalPlayers];
    coresPlayers[0] = "vermelha";
    coresPlayers[1] = "azul";

    /* Aloca e atribui missões dinamicamente para cada jogador */
    /* missoesAlocadas é um vetor de ponteiros para char* (cada um apontará para malloc'd string) */
    char** missoesAlocadas = (char**) calloc(totalPlayers, sizeof(char*));
    if (!missoesAlocadas) {
        fprintf(stderr, "Erro: nao foi possivel alocar vetor de missoes.\n");
        free(mapa);
        return 1;
    }

    /* Para imprimir a missão apenas UMA vez para cada jogador (no início) */
    for (int i = 0; i < totalPlayers; ++i) {
        /* 1) Sorteia um template para a missão para o jogador em um buffer temporário */
        char temp[200] = {0};
        atribuirMissao(temp, missoesPadrao, totalMissoesPadrao);

        /* 2) Construímos a string final contendo a cor do jogador e o texto (formato "cor|texto") */
        size_t needed = strlen(coresPlayers[i]) + 1 + strlen(temp) + 1;
        missoesAlocadas[i] = (char*) malloc(needed);
        if (!missoesAlocadas[i]) {
            fprintf(stderr, "Erro: malloc missao jogador %d\n", i);
            /* liberar o que já foi alocado */
            for (int j = 0; j < i; ++j) free(missoesAlocadas[j]);
            free(missoesAlocadas);
            free(mapa);
            return 1;
        }
        /* copia no formato "cor|texto" (isso permite que verificarMissao saiba qual cor pertence à missão) */
        strcpy(missoesAlocadas[i], coresPlayers[i]);
        strcat(missoesAlocadas[i], "|");
        strcat(missoesAlocadas[i], temp);

        /* Exibimos a missão apenas uma vez (no início) para o jogador */
        printf("=== Missao para %s (cor %s) ===\n", nomesPlayers[i], coresPlayers[i]);
        exibirMissao(missoesAlocadas[i]);
        printf("\n");
    }

    /* Loop de jogo simplificado: a cada turno, cada jogador pode tentar atacar (simulado).
       Ao final de cada turno (após ações dos jogadores), verificamos se algum jogador cumpriu a missão. */
    int turno = 1;
    int maxTurnos = 30; /* limite para terminar se ninguém vencer */
    int vencedor = -1;

    while (turno <= maxTurnos && vencedor == -1) {
        printf("\n=== Turno %d ===\n", turno);
        exibirMapa(mapa, tamanhoMapa);

        for (int p = 0; p < totalPlayers; ++p) {
            printf("\n-> Acoes de %s (cor %s)\n", nomesPlayers[p], coresPlayers[p]);

            /* Simples escolha aleatória: escolhe um território do jogador com tropas >=2 como atacante,
               e um território de cor diferente como defensor. Se não houver ataque válido, passa. */
            int idxAt = -1, idxDef = -1;
            /* procura atacante */
            for (int i = 0; i < tamanhoMapa; ++i) {
                if (strcmp(mapa[i].cor, coresPlayers[p]) == 0 && mapa[i].tropas >= 2) {
                    idxAt = i;
                    break;
                }
            }
            /* procura defensor (primeiro território com cor diferente e tropas >=0) */
            for (int j = 0; j < tamanhoMapa; ++j) {
                if (strcmp(mapa[j].cor, coresPlayers[p]) != 0) {
                    idxDef = j;
                    break;
                }
            }

            if (idxAt != -1 && idxDef != -1) {
                printf("%s ataca %s!\n", mapa[idxAt].nome, mapa[idxDef].nome);
                atacar(&mapa[idxAt], &mapa[idxDef]);
            } else {
                printf("%s nao encontrou ataque valido.\n", nomesPlayers[p]);
            }

            /* Após a ação do jogador p, verificamos se ele cumpriu sua missão (verificacao silenciosa) */
            int cumpriu = verificarMissao(missoesAlocadas[p], mapa, tamanhoMapa);
            if (cumpriu) {
                vencedor = p;
                break;
            }
        }

        turno++;
    }

    if (vencedor != -1) {
        printf("\n*** Vencedor: %s! (Missao cumprida) ***\n", nomesPlayers[vencedor]);
        printf("Missao cumprida: ");
        exibirMissao(missoesAlocadas[vencedor]);
    } else {
        printf("\nFim de jogo: nenhum jogador cumpriu a missao em %d turnos.\n", maxTurnos);
    }

    /* Liberamos toda a memória alocada dinamicamente */
    liberarMemoria(mapa, tamanhoMapa, missoesAlocadas, totalPlayers);

    return 0;
}

