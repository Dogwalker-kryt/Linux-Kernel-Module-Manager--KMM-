#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>


void KMM_log(const char *event, const char *arg) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%d-%m-%Y %H:%M", t);

    const char *sudo_user = getenv("SUDO_USER");
    const char *user_env  = getenv("USER");
    const char *username  = sudo_user ? sudo_user : user_env;

    if (!username) {
        fprintf(stderr, "[Logger Error] Could not determine username.\n");
        return;
    }

    struct passwd *pw = getpwnam(username);
    if (!pw) {
        fprintf(stderr, "[Logger Error] Failed to get home directory for user: %s\n", username);
        return;
    }

    char logDir[512];
    snprintf(logDir, sizeof(logDir), "%s/.local/share/KMM/", pw->pw_dir);

    // Create .local and .local/share if needed (probably will not ever happen)
    char localDir[512];
    snprintf(localDir, sizeof(localDir), "%s/.local", pw->pw_dir);
    mkdir(localDir, 0755);
    
    char shareDir[512];
    snprintf(shareDir, sizeof(shareDir), "%s/.local/share", pw->pw_dir);
    mkdir(shareDir, 0755);

    struct stat st;
    if (stat(logDir, &st) != 0) {
        if (mkdir(logDir, 0755) != 0 && errno != EEXIST) {
            fprintf(stderr, "[Logger Error] Failed to create log directory: %s (%s)\n",
                    logDir, strerror(errno));
            return;
        }
    }

    char logPath[600];
    snprintf(logPath, sizeof(logPath), "%s/log_data.log", logDir);


    FILE *f = fopen(logPath, "a");
    if (!f) {
        fprintf(stderr, "[Logger Error] Unable to open log file: %s (%s)\n",
                logPath, strerror(errno));
        return;
    }

    if (arg && strlen(arg) > 0) {
        fprintf(f, "[%s] event: %s | error: %s\n", timeStr, event, arg);
    } else {
        fprintf(f, "[%s] event: %s\n", timeStr, event);
    }

    fclose(f);
}
