#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_TEXT 1000
#define MAX_NAME 100
#define MAX_PATH 260
#define HISTORY_FILE "echo_history.txt"

typedef struct {
    char voice_name[MAX_NAME];
    char voice_id[MAX_NAME];
    char sample_path[MAX_PATH];
    int voice_type; /* 1 = Own voice, 2 = Authorised shared voice, 3 = Default */
} VoiceProfile;

typedef struct Message {
    char sender[20];
    char emotion[MAX_NAME];
    char content[MAX_TEXT];
    struct Message *next;
} Message;

typedef struct {
    char preferred_emotion[MAX_NAME];
    char current_emotion[MAX_NAME];
    int lock_emotion; /* 1 = User manually forces emotion, 0 = Adapt from message */
} EmotionProfile;

Message *head = NULL;
Message *tail = NULL;

VoiceProfile voice;
EmotionProfile emotion;

int running = 1;

/* Read a line safely and strip trailing newline */
void read_input(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);

    if (fgets(buffer, (int)size, stdin) == NULL) {
        running = 0;
        buffer[0] = '\0';
        return;
    }

    buffer[strcspn(buffer, "\r\n")] = '\0';
}

/* Read an integer safely with a fallback default */
int read_int_with_default(const char *prompt, int default_val) {
    char line[64];
    char *endptr;
    long val;

    read_input(prompt, line, sizeof(line));
    if (!running || line[0] == '\0') {
        return default_val;
    }

    val = strtol(line, &endptr, 10);
    if (endptr == line || *endptr != '\0') {
        return default_val;
    }

    return (int)val;
}

/* Convert ASCII string to lowercase in-place */
void to_lower_str(char *str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        str[i] = (char)tolower((unsigned char)str[i]);
    }
}

/* Check whether text contains a keyword (case-insensitive) */
int contains_word(const char *text, const char *word) {
    char a[MAX_TEXT];
    char b[MAX_TEXT];

    snprintf(a, sizeof(a), "%s", text);
    snprintf(b, sizeof(b), "%s", word);

    to_lower_str(a);
    to_lower_str(b);

    return strstr(a, b) != NULL;
}

/* Check if a file exists and is readable */
int file_exists(const char *path) {
    if (path == NULL || path[0] == '\0') {
        return 0;
    }
    FILE *fp = fopen(path, "rb");
    if (fp != NULL) {
        fclose(fp);
        return 1;
    }
    return 0;
}

/* Map emotion choice (1-6) to canonical label */
const char *emotion_from_choice(int choice) {
    switch (choice) {
        case 1: return "Comfort";
        case 2: return "Listening";
        case 3: return "Motivation";
        case 4: return "Reassurance";
        case 5: return "Playful";
        case 6: return "Advice";
        default: return "Comfort";
    }
}

void display_welcome(void) {
    printf("\n");
    printf("========================================\n");
    printf("              E C H O                   \n");
    printf("       YOUR PERSONAL AI COMPANION        \n");
    printf("========================================\n");
    printf("\nWelcome to ECHO.\n");
    printf("A voice, a conversation, a connection.\n\n");
}

void select_voice(void) {
    int choice;

    memset(&voice, 0, sizeof(voice));

    printf("\nWHOSE VOICE SHOULD ECHO USE?\n\n");
    printf("1. My own voice (sample file or profile ID)\n");
    printf("2. Someone else's authorised voice\n");
    printf("3. ECHO's default voice\n\n");

    choice = read_int_with_default("Select an option [1-3]: ", 3);
    voice.voice_type = choice;

    switch (choice) {
        case 1:
            snprintf(voice.voice_name, sizeof(voice.voice_name), "My Voice");
            read_input("Enter path to your voice sample (e.g. my_voice.wav, or blank): ",
                       voice.sample_path, sizeof(voice.sample_path));

            if (voice.sample_path[0] != '\0') {
                if (file_exists(voice.sample_path)) {
                    printf("Voice sample verified: %s\n", voice.sample_path);
                } else {
                    printf("Note: '%s' was not found yet; saved path for future synthesis.\n",
                           voice.sample_path);
                }
            }

            read_input("Enter voice profile ID (or leave blank): ",
                       voice.voice_id, sizeof(voice.voice_id));
            if (voice.voice_id[0] == '\0') {
                snprintf(voice.voice_id, sizeof(voice.voice_id), "CUSTOM_SELF");
            }
            break;

        case 2:
            snprintf(voice.voice_name, sizeof(voice.voice_name), "Shared Authorised Voice");
            printf("Only use a voice profile shared by its owner with explicit permission.\n");
            read_input("Enter authorised shared voice ID: ",
                       voice.voice_id, sizeof(voice.voice_id));
            if (voice.voice_id[0] == '\0') {
                snprintf(voice.voice_id, sizeof(voice.voice_id), "SHARED_UNSET");
            }
            read_input("Enter authorised sample path (or leave blank): ",
                       voice.sample_path, sizeof(voice.sample_path));
            break;

        default:
            voice.voice_type = 3;
            snprintf(voice.voice_name, sizeof(voice.voice_name), "ECHO Default Voice");
            snprintf(voice.voice_id, sizeof(voice.voice_id), "DEFAULT");
            break;
    }

    printf("\nActive voice: %s (ID: %s)\n", voice.voice_name, voice.voice_id);
}

void select_emotion(void) {
    int choice;
    const char *chosen;

    memset(&emotion, 0, sizeof(emotion));

    printf("\nHOW WOULD YOU LIKE ECHO TO TALK TO YOU?\n\n");
    printf("1. Comfort me\n");
    printf("2. Just listen\n");
    printf("3. Motivate me\n");
    printf("4. Reassure me\n");
    printf("5. Make me smile\n");
    printf("6. Help me think\n\n");

    choice = read_int_with_default("Select your preference [1-6]: ", 1);
    chosen = emotion_from_choice(choice);

    snprintf(emotion.preferred_emotion, sizeof(emotion.preferred_emotion), "%s", chosen);
    snprintf(emotion.current_emotion, sizeof(emotion.current_emotion), "%s", chosen);
    emotion.lock_emotion = 0;

    printf("\nECHO initial emotion: %s\n", emotion.preferred_emotion);
}

/* Update current emotion unless user locked a specific emotion */
void adapt_emotion(const char *message) {
    if (emotion.lock_emotion) {
        snprintf(emotion.current_emotion, sizeof(emotion.current_emotion),
                 "%s", emotion.preferred_emotion);
        return;
    }

    /* Start from preferred emotion if no strong cue is detected */
    snprintf(emotion.current_emotion, sizeof(emotion.current_emotion),
             "%s", emotion.preferred_emotion);

    if (contains_word(message, "sad") ||
        contains_word(message, "lonely") ||
        contains_word(message, "cry") ||
        contains_word(message, "upset")) {
        snprintf(emotion.current_emotion, sizeof(emotion.current_emotion), "Comfort");
    } else if (contains_word(message, "happy") ||
               contains_word(message, "excited") ||
               contains_word(message, "funny") ||
               contains_word(message, "laugh")) {
        snprintf(emotion.current_emotion, sizeof(emotion.current_emotion), "Playful");
    } else if (contains_word(message, "scared") ||
               contains_word(message, "worried") ||
               contains_word(message, "anxious") ||
               contains_word(message, "afraid")) {
        snprintf(emotion.current_emotion, sizeof(emotion.current_emotion), "Reassurance");
    } else if (contains_word(message, "motivate") ||
               contains_word(message, "goal") ||
               contains_word(message, "achieve") ||
               contains_word(message, "stuck")) {
        snprintf(emotion.current_emotion, sizeof(emotion.current_emotion), "Motivation");
    } else if (contains_word(message, "advice") ||
               contains_word(message, "decision") ||
               contains_word(message, "think") ||
               contains_word(message, "choose")) {
        snprintf(emotion.current_emotion, sizeof(emotion.current_emotion), "Advice");
    }
}

void add_message(const char *sender, const char *emo, const char *content) {
    Message *new_message = (Message *)malloc(sizeof(Message));
    if (new_message == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return;
    }

    snprintf(new_message->sender, sizeof(new_message->sender), "%s", sender);
    snprintf(new_message->emotion, sizeof(new_message->emotion), "%s", emo);
    snprintf(new_message->content, sizeof(new_message->content), "%s", content);
    new_message->next = NULL;

    if (head == NULL) {
        head = new_message;
        tail = new_message;
    } else {
        tail->next = new_message;
        tail = new_message;
    }
}

void save_history(const char *sender, const char *emo, const char *content) {
    static int warned_once = 0;
    FILE *file = fopen(HISTORY_FILE, "a");

    if (file == NULL) {
        if (!warned_once) {
            fprintf(stderr, "Warning: Could not write to %s.\n", HISTORY_FILE);
            warned_once = 1;
        }
        return;
    }

    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    char timestamp[30] = "Unknown time";

    if (local != NULL) {
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local);
    }

    fprintf(file, "[%s] [%s] %s: %s\n", timestamp, emo, sender, content);
    fclose(file);
}

/* Hook for external TTS / voice-cloning engine */
void trigger_voice_output(const VoiceProfile *vp, const char *emo, const char *text) {
    /* Connect your TTS/voice-cloning CLI, HTTP client, or C/Python bridge here,
       passing vp->voice_id, vp->sample_path, emo, and text. */
    (void)vp;
    (void)emo;
    (void)text;
}

void build_response_text(const char *emo, const char *user_msg, char *out, size_t size) {
    (void)user_msg;

    if (strcmp(emo, "Comfort") == 0) {
        snprintf(out, size,
                 "I'm sorry things feel heavy right now. You can tell me what's on your mind—I'm right here with you.");
    } else if (strcmp(emo, "Playful") == 0) {
        snprintf(out, size,
                 "Ooh, I love that energy! Tell me everything—what's got you smiling?");
    } else if (strcmp(emo, "Reassurance") == 0) {
        snprintf(out, size,
                 "Take a breath. We can handle this one small step at a time—you don't have to solve it all today.");
    } else if (strcmp(emo, "Motivation") == 0) {
        snprintf(out, size,
                 "You've got momentum waiting—pick one small win for the next ten minutes and let's build on it.");
    } else if (strcmp(emo, "Advice") == 0) {
        snprintf(out, size,
                 "Let's look at the options clearly. What outcome matters most to you right now?");
    } else if (strcmp(emo, "Listening") == 0) {
        snprintf(out, size,
                 "I'm listening. Take your time and share as much or as little as you want.");
    } else {
        snprintf(out, size,
                 "I'm here with you. Tell me more about what's going on.");
    }
}

void generate_and_record_response(const char *user_message) {
    char reply[MAX_TEXT];

    adapt_emotion(user_message);
    build_response_text(emotion.current_emotion, user_message, reply, sizeof(reply));

    printf("\n----------------------------------------\n");
    printf("ECHO (%s) [%s]: %s\n", voice.voice_name, emotion.current_emotion, reply);
    printf("----------------------------------------\n");

    trigger_voice_output(&voice, emotion.current_emotion, reply);

    add_message("ECHO", emotion.current_emotion, reply);
    save_history("ECHO", emotion.current_emotion, reply);
}

/* Handle slash commands inside chat: /emotion, /auto, /voice, /history */
int handle_command(const char *input) {
    if (strcmp(input, "/auto") == 0) {
        emotion.lock_emotion = 0;
        snprintf(emotion.current_emotion, sizeof(emotion.current_emotion),
                 "%s", emotion.preferred_emotion);
        printf("Emotion mode set to AUTO (adapting to conversation; base: %s).\n",
               emotion.preferred_emotion);
        return 1;
    }

    if (strncmp(input, "/emotion", 8) == 0 && (input[8] == '\0' || input[8] == ' ')) {
        int choice;
        if (input[8] == ' ') {
            choice = (int)strtol(input + 9, NULL, 10);
        } else {
            printf("1=Comfort  2=Listening  3=Motivation  4=Reassurance  5=Playful  6=Advice\n");
            choice = read_int_with_default("Choose emotion [1-6]: ", 1);
        }
        const char *chosen = emotion_from_choice(choice);
        snprintf(emotion.preferred_emotion, sizeof(emotion.preferred_emotion), "%s", chosen);
        snprintf(emotion.current_emotion, sizeof(emotion.current_emotion), "%s", chosen);
        emotion.lock_emotion = 1;
        printf("Emotion locked to: %s (use /auto to return to automatic adaptation).\n",
               emotion.current_emotion);
        return 1;
    }

    if (strcmp(input, "/voice") == 0) {
        select_voice();
        return 1;
    }

    if (strcmp(input, "/history") == 0) {
        Message *cur = head;
        printf("\n--- Session History ---\n");
        while (cur != NULL) {
            printf("[%s] %s: %s\n", cur->emotion, cur->sender, cur->content);
            cur = cur->next;
        }
        printf("-----------------------\n");
        return 1;
    }

    return 0;
}

void start_conversation(void) {
    char message[MAX_TEXT];

    printf("\n========================================\n");
    printf("          ECHO CONVERSATION             \n");
    printf("========================================\n");
    printf("Voice: %s (%s)\n", voice.voice_name, voice.voice_id);
    printf("Emotion: %s (Auto-adapt)\n", emotion.current_emotion);
    printf("Commands: /emotion [1-6], /auto, /voice, /history, or 'exit'\n");

    while (running) {
        read_input("\nYou: ", message, sizeof(message));

        if (!running) {
            break;
        }
        if (message[0] == '\0') {
            continue;
        }

        char lower_cmd[MAX_TEXT];
        snprintf(lower_cmd, sizeof(lower_cmd), "%s", message);
        to_lower_str(lower_cmd);

        if (strcmp(lower_cmd, "exit") == 0 || strcmp(lower_cmd, "quit") == 0) {
            printf("\nECHO: Take care. I'll be here whenever you want to talk again.\n");
            break;
        }

        if (message[0] == '/' && handle_command(message)) {
            continue;
        }

        add_message("User", emotion.current_emotion, message);
        save_history("User", emotion.current_
