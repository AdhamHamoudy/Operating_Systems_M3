#pragma once
#include <set>
#include <unordered_map>

// פונקציה שנקראת כשה־fd חם (read-ready)
typedef void* (*reactorFunc)(int fd);

// מחלקת ה־Reactor הפנימית
class Reactor {
public:
    Reactor();
    ~Reactor();

    bool addFd(int fd, reactorFunc func);     // הוסף fd עם פונקציה
    bool removeFd(int fd);                    // הסר fd
    void run();                               // הפעל את לולאת ה־select
    void stop();                              // עצור את הלולאה
private:
    std::set<int> fds;                        // רשימת קבצים מנוטרים
    std::unordered_map<int, reactorFunc> handlers;  // מיפוי fd → פונקציה
    bool running;                             // האם ה־Reactor רץ?
};

// ממשק C כפי שנדרש ע"י התרגיל
extern "C" {
    void* startReactor();                               // יצירת ריאקטור חדש
    int addFdToReactor(void* reactor, int fd, reactorFunc func);
    int removeFdFromReactor(void* reactor, int fd);
    int stopReactor(void* reactor);                     // רק עוצר – לא מוחק
    void runReactor(void* reactor);                     // חייב להיקרא ע"י המשתמש
}
