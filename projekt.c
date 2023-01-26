#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    FILE *ptr;
    char buf[100];

    char fifo_name[100] = "";
    // bierzemy nazwe do fifo
    char *name = argv[1];
    char *first_word;
    char *second_word;
    char *third_word;

    // otweiramy  i sprawdzamy plik
    ptr = fopen("fifo.txt", "r");
    if (NULL == ptr)
    {
        perror("blad przy otwieraniu pliku");
        exit(1);
    }
    while (fgets(buf, 20, ptr) != NULL)
    {
        first_word = strtok(buf, " ");
        second_word = strtok(NULL, " ");
        third_word = strtok(NULL, "\n");
        // szukamy jaka nazwe wprowadzul uzytkownik
        if (strcmp(first_word, name) == 0)
        {
            strcpy(fifo_name, third_word);
        }
    }
    if (strcmp(fifo_name, "") == 0)
    {
        perror("niepoprawna nazwa procesu");
        exit(1);
    }

    int status;
    int fifo_queue;
    int fifo_queue_child;
    int id;
    int id_main;
    int status_proccess_to_create;
    int name_proccess_to_create;
    char name_to_proccess[100];
    char command[100];
    char fifo_to_proccess[100];
    char buf_message[500];
    int rs;

    id_main = fork();
    if (id_main == 0)
    {
        // tworzenie glownej kolejki danego procesu
        status = mkfifo(fifo_name, 0640);

        printf("Status kplejki %d \n", status);
        if (status == -1)
        {
            perror("blad orzy tworzeniu koljki");
            exit(1);
        }
        // otweiranie glownej kolejki
        fifo_queue = open(fifo_name, O_RDONLY);
        if (fifo_queue == -1)
        {
            perror("blad orzy otwierniu koljki:");
            exit(1);
        }
        while (1)
        {
            // zczytywanie czy cos przyszlo do kolejki

            rs = read(fifo_queue, buf_message, 500);
            if (rs == -1)
            {
                perror("blad przy zczytaniu z kolejki");
                close(fifo_queue);
                unlink(fifo_name);
                exit(1);
            }
            if (rs > 0)
            {
                printf("dostaleem: %s\n", buf_message);
                char *command_to_do;
                char *fifo_to_create_in_procces;
                command_to_do = strtok(buf_message, ",");
                fifo_to_create_in_procces = strtok(NULL, " ");
                fifo_queue_child = open(fifo_to_create_in_procces, O_WRONLY);
                if (fifo_queue_child == -1)
                {
                    perror("blad przy otwieraniu kolejki");
                    close(fifo_queue);
                    unlink(fifo_name);
                }
                int id;
                id = fork();
                if (id == 0)
                {
                    close(STDOUT_FILENO);
                    dup(fifo_queue_child);
                    close(fifo_queue_child);
                    execlp("/bin/bash", "/bin/bash", "-c", command_to_do, (char *)0);
                    exit(0);
                }

                close(fifo_queue_child);
            }

            sleep(1);
        }
    }
    else
    {
        while (1)
        {

            printf("podaj komende: \n");
           
            char str[100];
            scanf("%[^\n]%*c", str);
            if (strcmp(str, "exit") == 0)
            {
                unlink(fifo_name);
                exit(1);
            }

            char words[3][100] = {"", "", ""};
            int index = 0;
            int quote = -1;
            char buf_message[500] = "";
            char buf_child[100] = "";
            char fifo_to_write[100] = "";

            // zamiana komendy na 3 osobne stringi
            for (int i = 0; i < strlen(str); i++)
            {
                if (str[i] == '"')
                {
                    quote *= -1;
                    continue;
                }
                if (str[i] == ' ' && quote == -1)
                {
                    index++;
                    if (index > 2)
                    {
                        printf("za duzo arg\n");
                        break;
                    }
                    continue;
                }
                strncat(words[index], &str[i], 1);
            }
            strcpy(name_to_proccess, words[0]);
            strcpy(command, words[1]);
            strcpy(fifo_to_proccess, words[2]);
            if (strcmp(name, name_to_proccess) == 0)
            {
                perror("nie mozna zlecic procesowi proszacemu o wykonanie wykonania");
                close(fifo_queue);
                unlink(fifo_name);
                exit(1);
            }

            // robienie kolejki o nazwie wprowadzonej
            status_proccess_to_create = mkfifo(fifo_to_proccess, 0640);
            if (status_proccess_to_create == -1)
            {
                perror("Blad w tworzeniu kolejki:");
                close(fifo_queue);
                unlink(fifo_name);
                exit(1);
            }
            // tutj rdwr, bo inaczej proces by się blokował i nie miał jkaby przesłać nazwy tego procesu dalej
            //                to moze odkomentowac
            name_proccess_to_create = open(fifo_to_proccess, O_RDWR);
            if (name_proccess_to_create == -1)
            {
                perror("otwarcie kolejki");
                close(fifo_queue);
                unlink(fifo_name);
            }
            ptr = fopen("fifo.txt", "r");
            if (NULL == ptr)
            {
                perror("blad przy otwieraniu pliku");
                close(fifo_queue);
                unlink(fifo_name);
                exit(1);
            }
            while (fgets(buf_child, 20, ptr) != NULL)
            {
                first_word = strtok(buf_child, " ");
                second_word = strtok(NULL, " ");
                third_word = strtok(NULL, "\n");

                // szukamy jaka nazwe wprowadzul uzytkownik
                if (strcmp(first_word, name_to_proccess) == 0)
                {
                    strcpy(fifo_to_write, third_word);
                }
            }

            // otweiramy glowna kolejke
            fifo_queue = open(fifo_to_write, O_WRONLY);
            if (fifo_queue == -1)
            {
                perror("blad przy otwarciu pliku do pisania");
                close(fifo_queue);
                unlink(fifo_name);
                unlink(fifo_to_proccess);
                exit(1);
            }
            char destination[20] = "";
            strcpy(destination, fifo_to_proccess);
            strcat(command, ",");
            strcat(command, fifo_to_proccess);
            if (write(fifo_queue, command, 100) == -1)
            {
                perror("blad w zapisie do kolejki");
                close(fifo_queue);
                unlink(fifo_name);
                exit(1);
            }

            // odbieramy z kolejki wiadomosc i ja wyswietlamy
            name_proccess_to_create = open(destination, O_RDONLY);
            if (name_proccess_to_create == -1)
            {
                perror("blad przy zapsiie pliku do odczytu");
                close(fifo_queue);
                unlink(fifo_name);
                exit(1);
            }
            printf("odbieramy: %s\n", destination);
            if (read(name_proccess_to_create, buf_message, 500) == -1)
            {
                perror("blad przy odczycie z kolejki");
                close(fifo_queue);
                unlink(fifo_name);
                exit(1);
            }
            if (strcmp(buf_message, "") == -1)
            {
                close(fifo_queue);
                unlink(destination);
                unlink(fifo_name);
                exit(1);
            }

            printf("%s\n", buf_message);
            close(name_proccess_to_create);
            unlink(destination);
            close(fifo_queue);
        }
    }
    return 0;
}
