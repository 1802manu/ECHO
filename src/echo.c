
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_TEXT 1000
#define MAX_NAME 100
#define HISTORY_FILE "echo_history.txt"

typedef struct {
    char voice_name[MAX_NAME];
    char voice_id[MAX_NAME];
    int voice_type;
} VoiceProfile;

typedef struct Message {
    char sender[20];
    char content[MAX_TEXT];
    struct Message *next;
} Message;

typedef struct {
    char selected_emotion[MAX_NAME];
    char current_emotion[MAX_NAME];
} EmotionProfile;

Message *head = NULL;
Message *tail = NULL;

VoiceProfile voice;
EmotionProfile emotion;

int running = 1;

/* Read user input safely */
void read_input(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);

    if (fgets(buffer, (int)size, stdin) == NULL) {
        running = 0;
        buffer[0] = '\0';
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';
}

/* Check whether text contains a keyword */
int contains_word(const char *text, const char *word) {
    char a[MAX_TEXT];
    char b[MAX_TEXT];

    snprintf(a, sizeof(a), "%s", text);
    snprintf(b, sizeof(b), "%s", word);

    for (int i = 0; a[i]; i++)
        a[i] = (char)tolower((unsigned char)a[i]);

    for (int i = 0; b[i]; i++)
        b[i] = (char)tolower((unsigned char)b[i]);

    return strstr(a, b) != NULL;
}

/* Display ECHO branding */
void display_welcome(void) {
    printf("\n");
    printf("========================================\n");
    printf("              E C H O                   \n");
    printf("       YOUR PERSONAL AI COMPANION        \n");
    printf("========================================\n");

    printf("\nWelcome to ECHO.\n");
    printf("A voice, a conversation, a connection.\n\n");
}

/* Select voice */
void select_voice(void) {
    int choice;

    printf("\nWHO WOULD YOU LIKE TO TALK TO?\n\n");

    printf("1. My own voice\n");
    printf("2. Someone else's authorised voice\n");
    printf("3. ECHO's default voice\n");

    printf("\nSelect an option: ");

    if (scanf("%d", &choice) != 1) {
        while (getchar() != '\n' && !feof(stdin));
        choice = 3;
    } else {
        while (getchar() != '\n' && !feof(stdin));
    }

    voice.voice_type = choice;

    switch (choice) {

        case 1:
            strcpy(voice.voice_name, "My Voice");

            printf("\nYour voice profile must be created ");
            printf("through the authorised voice service.\n");

            read_input(
                "Enter your voice profile ID (or leave blank): ",
                voice.voice_id,
                sizeof(voice.voice_id)
            );

            break;

        case 2:
            strcpy(voice.voice_name, "Shared Voice");

            printf("\nOnly use a voice profile shared by ");
            printf("its owner with permission.\n");

            read_input(
                "Enter the authorised shared voice ID: ",
                voice.voice_id,
                sizeof(voice.voice_id)
            );

            break;

        default:
            voice.voice_type = 3;
            strcpy(voice.voice_name, "ECHO Default Voice");
            strcpy(voice.voice_id, "DEFAULT");

            break;
    }

    printf("\nSelected voice: %s\n", voice.voice_name);
}

/* Select initial emotional preference */
void select_emotion(void) {
    int choice;

    printf("\nHOW WOULD YOU LIKE ECHO TO TALK TO YOU?\n\n");

    printf("1. Comfort me\n");
    printf("2. Just listen\n");
    printf("3. Motivate me\n");
    printf("4. Reassure me\n");
    printf("5. Make me smile\n");
    printf("6. Help me think\n");

    printf("\nSelect your preference: ");

    if (scanf("%d", &choice) != 1) {
        while (getchar() != '\n' && !feof(stdin));
        choice = 1;
    } else {
        while (getchar() != '\n' && !feof(stdin));
    }

    switch (choice) {
        case 1:
            strcpy(emotion.selected_emotion, "Comfort");
            break;

        case 2:
            strcpy(emotion.selected_emotion, "Listening");
            break;

        case 3:
            strcpy(emotion.selected_emotion, "Motivation");
            break;

        case 4:
            strcpy(emotion.selected_emotion, "Reassurance");
            break;

        case 5:
            strcpy(emotion.selected_emotion, "Playful");
            break;

        case 6:
            strcpy(emotion.selected_emotion, "Advice");
            break;

        default:
            strcpy(emotion.selected_emotion, "Comfort");
    }

    strcpy(
        emotion.current_emotion,
        emotion.selected_emotion
    );

    printf("\nECHO will initially use: %s\n",
           emotion.selected_emotion);
}

/* Basic emotional context detection */
void adapt_emotion(const char *message) {

    if (contains_word(message, "sad") ||
        contains_word(message, "lonely") ||
        contains_word(message, "cry") ||
        contains_word(message, "upset")) {

        strcpy(emotion.current_emotion, "Comfort");
    }

    else if (contains_word(message, "happy") ||
             contains_word(message, "excited") ||
             contains_word(message, "funny")) {

        strcpy(emotion.current_emotion, "Playful");
    }

    else if (contains_word(message, "scared") ||
             contains_word(message, "worried") ||
             contains_word(message, "anxious")) {

        strcpy(emotion.current_emotion, "Reassurance");
    }

    else if (contains_word(message, "motivate") ||
             contains_word(message, "goal") ||
             contains_word(message, "achieve")) {

        strcpy(emotion.current_emotion, "Motivation");
    }

    else if (contains_word(message, "advice") ||
             contains_word(message, "decision") ||
             contains_word(message, "think")) {

        strcpy(emotion.current_emotion, "Advice");
    }

    else {
        /* Keep the existing emotional preference */
    }
}

/* Add a conversation message to linked list */
void add_message(const char *sender, const char *content) {

    Message *new_message =
        (Message *)malloc(sizeof(Message));

    if (new_message == NULL) {
        printf("Memory allocation failed.\n");
        return;
    }

    snprintf(new_message->sender,
             sizeof(new_message->sender),
             "%s", sender);

    snprintf(new_message->content,
             sizeof(new_message->content),
             "%s", content);

    new_message->next = NULL;

    if (head == NULL) {
        head = new_message;
        tail = new_message;
    } else {
        tail->next = new_message;
        tail = new_message;
    }
}

/* Save conversation locally */
void save_history(const char *sender, const char *content) {

    FILE *file = fopen(HISTORY_FILE, "a");

    if (file == NULL) {
        printf("Could not save conversation history.\n");
        return;
    }

    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    char timestamp[30] = "Unknown time";

    if (local != NULL) {
        strftime(timestamp, sizeof(timestamp),
                 "%Y-%m-%d %H:%M:%S", local);
    }

    fprintf(file, "[%s] %s: %s\n",
            timestamp, sender, content);

    fclose(file);
}

/* Generate a basic prototype response */
void generate_response(const char *message) {

    adapt_emotion(message);

    printf("\n----------------------------------------\n");
    printf("ECHO [%s]: ", emotion.current_emotion);

    if (strcmp(emotion.current_emotion, "Comfort") == 0) {

        printf("I'm sorry things feel difficult right now. ");
        printf("You can tell me what's on your mind. ");
        printf("I'll listen without judging.\n");
    }

    else if (strcmp(emotion.current_emotion, "Playful") == 0) {

        printf("That sounds wonderful! ");
        printf("Tell me more. I want to hear the whole story!\n");
    }

    else if (strcmp(emotion.current_emotion, "Reassurance") == 0) {

        printf("Let's take things one step at a time. ");
        printf("You don't have to figure everything out at once.\n");
    }

    else if (strcmp(emotion.current_emotion, "Motivation") == 0) {

        printf("You have a goal. Now let's work towards it. ");
        printf("Small steps count too!\n");
    }

    else if (strcmp(emotion.current_emotion, "Advice") == 0) {

        printf("Let's understand the situation first. ");
        printf("What matters most to you in this decision?\n");
    }

    else if (strcmp(emotion.current_emotion, "Listening") == 0) {

        printf("I'm listening. Take your time. ");
        printf("You can say as much or as little as you want.\n");
    }

    else {

        printf("I'm here. Tell me more about your day.\n");
    }

    printf("----------------------------------------\n");

    add_message("ECHO", "Prototype response");
    save_history("ECHO", "Prototype response");
}

/* Conversation loop */
void start_conversation(void) {

    char message[MAX_TEXT];

    printf("\n========================================\n");
    printf("          ECHO CONVERSATION             \n");
    printf("========================================\n");

    printf("Voice: %s\n", voice.voice_name);
    printf("Initial preference: %s\n",
           emotion.selected_emotion);

    printf("\nType 'exit' to end the conversation.\n");

    while (running) {

        read_input("\nYou: ", message, sizeof(message));

        if (!running)
            break;

        if (strlen(message) == 0)
            continue;

        if (strcmp(message, "exit") == 0 ||
            strcmp(message, "EXIT") == 0) {

            printf("\nECHO: Take care. I'll be here ");
            printf("when you want to talk again.\n");

            break;
        }

        add_message("User", message);
        save_history("User", message);

        generate_response(message);
    }
}

/* Release allocated memory */
void free_history(void) {

    Message *current = head;

    while (current != NULL) {

        Message *next = current->next;

        free(current);
        current = next;
    }

    head = NULL;
    tail = NULL;
}

/* Main application */
int main(void) {

    display_welcome();

    select_voice();

    select_emotion();

    start_conversation();

    free_history();

    printf("\nThank you for using ECHO.\n");
    printf("========================================\n");

    return 0;
}
