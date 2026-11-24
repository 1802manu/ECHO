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
    int voice_type;
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
    int locked;
} EmotionProfile;

static Message *head = NULL;
static Message *tail = NULL;
static VoiceProfile voice;
static EmotionProfile emotion;
static int running = 1;

static void read_line(const char *prompt, char *buf, size_t size) {
    printf("%s", prompt);
    fflush(stdout);

    if (fgets(buf, (int)size, stdin) == NULL) {
        running = 0;
        buf[0] = '\0';
        return;
    }
    buf[strcspn(buf, "\r\n")] = '\0';
}

static int read_choice(const char *prompt, int fallback) {
    char line[64];
    char *end;
    long val;

    read_line(prompt, line, sizeof(line));
    if (!running || line[0] == '\0') {
        return fallback;
    }

    val = strtol(line, &end, 10);
    if (end == line || *end != '\0') {
        return fallback;
    }
    return (int)val;
}

static void to_lower(char *s) {
    for (size_t i = 0; s[i] != '\0'; i++) {
        s[i] = (char)tolower((unsigned char)s[i]);
    }
}

static int has_keyword(const char *text, const char *kw) {
    char a[MAX_TEXT];
    char b[MAX_TEXT];

    snprintf(a, sizeof(a), "%s", text);
    snprintf(b, sizeof(b), "%s", kw);
    to_lower(a);
    to_lower(b);
    return strstr(a, b) != NULL;
}

static int file_readable(const char *path) {
    FILE *fp;
    if (path == NULL || path[0] == '\0') {
        return 0;
    }
    fp = fopen(path, "rb");
    if (fp != NULL) {
        fclose(fp);
        return 1;
    }
    return 0;
}

static const char *map_emotion(int choice) {
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

static void show_banner(void) {
    printf("\n========================================\n");
    printf("              E C H O                   \n");
    printf("       YOUR PERSONAL AI COMPANION        \n");
    printf("========================================\n");
    printf("Welcome to ECHO.\n\n");
}

static void setup_voice(void) {
    int choice;

    memset(&voice, 0, sizeof(voice));
    printf("1. My own voice\n");
    printf("2. Someone else's authorised voice\n");
    printf("3. ECHO's default voice\n");

    choice = read_choice("\nSelect voice option [1-3]: ", 3);
    voice.voice_type = choice;

    if (choice == 1) {
        snprintf(voice.voice_name, sizeof(voice.voice_name), "My Voice");
        read_line("Voice sample file path (optional): ", voice.sample_path, sizeof(voice.sample_path));
        if (voice.sample_path[0] != '\0' && !file_readable(voice.sample_path)) {
            printf("Sample file not found yet; path saved for synthesis.\n");
        }
        read_line("Voice profile ID (optional): ", voice.voice_id, sizeof(voice.voice_id));
        if (voice.voice_id[0] == '\0') {
            snprintf(voice.voice_id, sizeof(voice.voice_id), "SELF_VOICE");
        }
    } else if (choice == 2) {
        snprintf(voice.voice_name, sizeof(voice.voice_name), "Shared Authorised Voice");
        read_line("Authorised voice ID: ", voice.voice_id, sizeof(voice.voice_id));
        if (voice.voice_id[0] == '\0') {
            snprintf(voice.voice_id, sizeof(voice.voice_id), "SHARED_VOICE");
        }
        read_line("Authorised sample path (optional): ", voice.sample_path, sizeof(voice.sample_path));
    } else {
        voice.voice_type = 3;
        snprintf(voice.voice_name, sizeof(voice.voice_name), "ECHO Default Voice");
        snprintf(voice.voice_id, sizeof(voice.voice_id), "DEFAULT");
    }

    printf("Active voice: %s (%s)\n", voice.voice_name, voice.voice_id);
}

static void setup_emotion(void) {
    int choice;
    const char *label;

    memset(&emotion, 0, sizeof(emotion));
    printf("\n1. Comfort me\n");
    printf("2. Just listen\n");
    printf("3. Motivate me\n");
    printf("4. Reassure me\n");
    printf("5. Make me smile\n");
    printf("6. Help me think\n");

    choice = read_choice("\nSelect initial emotion [1-6]: ", 1);
    label = map_emotion(choice);

    snprintf(emotion.preferred_emotion, sizeof(emotion.preferred_emotion), "%s", label);
    snprintf(emotion.current_emotion, sizeof(emotion.current_emotion), "%s", label);
    emotion.locked = 0;

    printf("Initial emotion: %s\n", emotion.preferred_emotion);
}

static void update_emotion(const char *msg) {
    if (emotion.locked) {
        snprintf(emotion.current_emotion, sizeof(emotion.current_emotion),
                 "%s", emotion.preferred_emotion);
        return;
    }

    snprintf(emotion.current_emotion, sizeof(emotion.current_emotion),
             "%s", emotion.preferred_emotion);

    if (has_keyword(msg, "sad") || has_keyword(msg, "lonely") ||
        has_keyword(msg, "cry") || has_keyword(msg, "upset")) {
        snprintf(emotion.current_emotion, sizeof(emotion.current_emotion), "Comfort");
    } else if (has_keyword(msg, "happy") || has_keyword(msg, "excited") ||
               has_keyword(msg, "funny") || has_keyword(msg, "laugh")) {
        snprintf(emotion.current_emotion, sizeof(emotion.current_emotion), "Playful");
    } else if (has_keyword(msg, "scared") || has_keyword(msg, "worried") ||
               has_keyword(msg, "anxious") || has_keyword(msg, "afraid")) {
        snprintf(emotion.current_emotion, sizeof(emotion.current_emotion), "Reassurance");
    } else if (has_keyword(msg, "motivate") || has_keyword(msg, "goal") ||
               has_keyword(msg, "achieve") || has_keyword(msg, "stuck")) {
        snprintf(emotion.current_emotion, sizeof(emotion.current_emotion), "Motivation");
    } else if (has_keyword(msg, "advice") || has_keyword(msg, "decision") ||
               has_keyword(msg, "think") || has_keyword(msg, "choose")) {
        snprintf(emotion.current_emotion, sizeof(emotion.current_emotion), "Advice");
    }
}

static void compose_reply(const char *emo, char *out, size_t size) {
    if (strcmp(emo, "Comfort") == 0) {
        snprintf(out, size, "I'm sorry things feel heavy right now. I'm here to listen without judgment.");
    } else if (strcmp(emo, "Playful") == 0) {
        snprintf(out, size, "Love that energy! Tell me more—what's making you smile?");
    } else if (strcmp(emo, "Reassurance") == 0) {
        snprintf(out, size, "Take a steady breath. One small step at a time is enough for now.");
    } else if (strcmp(emo, "Motivation") == 0) {
        snprintf(out, size, "You've got this. Pick one small task for the next few minutes and start there.");
    } else if (strcmp(emo, "Advice") == 0) {
        snprintf(out, size, "Let's break it down clearly. What matters most to you in this choice?");
    } else if (strcmp(emo, "Listening") == 0) {
        snprintf(out, size, "I'm listening. Take your time and share whatever feels right.");
    } else {
        snprintf(out, size, "I'm here with you. Tell me more about your day.");
    }
}

static void append_node(const char *sender, const char *emo, const char *text) {
    Message *node = (Message *)malloc(sizeof(Message));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return;
    }

    snprintf(node->sender, sizeof(node->sender), "%s", sender);
    snprintf(node->emotion, sizeof(node->emotion), "%s", emo);
    snprintf(node->content, sizeof(node->content), "%s", text);
    node->next = NULL;

    if (head == NULL) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }
}

static void write_log(const char *sender, const char *emo, const char *text) {
    static int warned = 0;
    FILE *fp = fopen(HISTORY_FILE, "a");
    if (fp == NULL) {
        if (!warned) {
            fprintf(stderr, "Warning: Could not open %s for writing.\n", HISTORY_FILE);
            warned = 1;
        }
        return;
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char stamp[32] = "Unknown time";

    if (tm_info != NULL) {
        strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", tm_info);
    }

    fprintf(fp, "[%s] [%s] %s: %s\n", stamp, emo, sender, text);
    fclose(fp);
}

static int run_command(const char *cmd) {
    if (strcmp(cmd, "/auto") == 0) {
        emotion.locked = 0;
        snprintf(emotion.current_emotion, sizeof(emotion.current_emotion),
                 "%s", emotion.preferred_emotion);
        printf("Emotion set to AUTO (base: %s).\n", emotion.preferred_emotion);
        return 1;
    }
    if (strncmp(cmd, "/emotion", 8) == 0 && (cmd[8] == '\0' || cmd[8] == ' ')) {
        int choice = (cmd[8] == ' ') ? (int)strtol(cmd + 9, NULL, 10)
                                  : read_choice("Emotion [1-6]: ", 1);
        const char *label = map_emotion(choice);
        snprintf(emotion.preferred_emotion, sizeof(emotion.preferred_emotion), "%s", label);
        snprintf(emotion.current_emotion, sizeof(emotion.current_emotion), "%s", label);
        emotion.locked = 1;
        printf("Emotion locked to %s.\n", emotion.current_emotion);
        return 1;
    }
    if (strcmp(cmd, "/voice") == 0) {
        setup_voice();
        return 1;
    }
    if (strcmp(cmd, "/history") == 0) {
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

static void chat_loop(void) {
    char msg[MAX_TEXT];
    char lower[MAX_TEXT];
    char reply[MAX_TEXT];

    printf("\n--- ECHO Chat Started (type 'exit' to quit; commands: /emotion, /auto, /voice, /history) ---\n");

    while (running) {
        read_line("\nYou: ", msg, sizeof(msg));
        if (!running) {
            break;
        }
        if (msg[0] == '\0') {
            continue;
        }

        snprintf(lower, sizeof(lower), "%s", msg);
        to_lower(lower);
        if (strcmp(lower, "exit") == 0 || strcmp(lower, "quit") == 0) {
            printf("\nECHO: Take care. Talk whenever you're ready.\n");
            break;
        }

        if (msg[0] == '/' && run_command(msg)) {
            continue;
        }

        update_emotion(msg);
        append_node("User", emotion.current_emotion, msg);
        write_log("User", emotion.current_emotion, msg);

        compose_reply(emotion.current_emotion, reply, sizeof(reply));
        printf("\nECHO (%s) [%s]: %s\n", voice.voice_name, emotion.current_emotion, reply);

        append_node("ECHO", emotion.current_emotion, reply);
        write_log("ECHO", emotion.current_emotion, reply);
    }
}

static void cleanup_nodes(void) {
    Message *cur = head;
    while (cur != NULL) {
        Message *next = cur->next;
        free(cur);
        cur = next;
    }
    head = NULL;
    tail = NULL;
}

int main(void) {
    show_banner();
    setup_voice();
    setup_emotion();
    chat_loop();
    cleanup_nodes();
    printf("\nSession ended.\n");
    return 0;
}
