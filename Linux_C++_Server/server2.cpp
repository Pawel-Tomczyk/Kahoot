#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

class Player{
public:
    std::string nick;
    int wynik;
    bool ans;
};

class Pytanie{
public:
    std::string pytanie;
    std::string odpa;
    std::string odpb;
    std::string odpc;
    std::string odpd;
    char poprawna;
    
    int answers; // ile osób odpowiedziało
    bool reduced; // czy czas został zredukowny
    bool used;  // czy pytanie jest już zużyte
};

class Quiz{
public:
    std::string nazwa;
    std::string code;

    int creator_fd;
    std::string creator_nick;

    int czas;
    int reducedCzas;

    std::vector<Pytanie> pytania;
    int current_question;

    std::unordered_map<int, Player> gracze;
    Player best_player;

    bool inProgress;
    bool noHost;
    bool empty;


    Quiz(std::string nazwa, int creator_fd, std::string creator_nick, int czas, int reducedCzas, std::string code)
    : nazwa(nazwa), creator_fd(creator_fd), creator_nick(creator_nick), czas(czas), reducedCzas(reducedCzas), code(code)
    {
        current_question = 0;
        inProgress = false;
        noHost = false;
        empty = false;
        Player p;
        p.nick = "none";
        p.wynik = -1;
        p.ans = false;
        best_player = p;
    }

    ~Quiz() {
        pytania.clear();
        gracze.clear();
    }

    void addQuestion(Pytanie p){
        pytania.push_back(p);
    }

    void sendToAllPlayers(std::string message){
        if (gracze.empty())
        {
            return;
        }
        
        for (auto& player : gracze){
            write(player.first, message.c_str(), message.size());
        }
    }

    void sendToPlayersExcept(int fd, std::string message){
        if (gracze.empty())
        {
            return;
        }
        
        for (auto& player : gracze){
            if (player.first != fd){
                write(player.first, message.c_str(), message.size());
            }
        }
    }

    void sendToPlayer(int fd, std::string message){ // unikać
        write(fd, message.c_str(), message.size());
    }

    void sendToHost(std::string message){
        if (noHost){
            return;
        }
        write(creator_fd, message.c_str(), message.size());
    }

    void sendPlayersList(int fd){
        std::string message;
        for (auto& player : gracze){
            message = "player\t" + player.second.nick + "\n";
            write(fd, message.c_str(), message.size());
        }
    }

    void addPlayer(int fd, std::string nick){
        std::string message = "player\t" + nick + "\n";
        this->sendToAllPlayers(message);
        this->sendToHost(message);
        Player p;
        p.nick = nick;
        p.wynik = 0;
        p.ans = false;
        gracze.insert({fd, p});
    }

    void endGame(){
        std::string message = "leader\t" + best_player.nick + "\t" + std::to_string(best_player.wynik) +"\n";
        this->sendToAllPlayers(message);
        this->sendToHost(message);
    }

    void removePlayer(int fd){
        Player lastLook = gracze[fd];
        std::string nick = lastLook.nick;
        gracze.erase(fd);
        if (gracze.empty()){  
            if(best_player.wynik == -1){
                best_player = lastLook;
            }      
            if (inProgress){
                this->endGame();
                return;
            }            
        }
        if(!inProgress){
            this->sendToAllPlayers("playerDisconnected\t" + nick + "\n");
            this->sendToHost("playerDisconnected\t" + nick + "\n");
        }
    }

    void removeHost(){
        noHost = true;
        this->sendToAllPlayers("noHost\n");
    }

    void showQuestion(){
        try{
            Pytanie current = pytania[current_question];
            std::string message = "question\t" + current.pytanie + "\t" + current.odpa + "\t" + current.odpb + "\t" + current.odpc + "\t" + current.odpd + "\t" + std::to_string(czas) + "\n";
            this->sendToAllPlayers(message);
            message = "question\t" + current.pytanie + "\t" + current.poprawna + "\t" + std::to_string(czas) + "\n";
        }
        catch(const std::exception& e){
            std::string message = "Sth wrong with showing question\n";
            return;
        }
    }

    void nextQuestion(){
        if(!pytania[current_question].used){
            return;
        }
        current_question++;
        if (current_question >= pytania.size()){
            this->endGame();
            return;
        }
        this->showQuestion();
    }

    void resetQuestions(){
        for (auto& question : pytania){
            question.answers = 0;
            question.reduced = false;
            question.used = false;
        }
    }

    void answerQuestion(int fd, std::string answer){
        Pytanie* current = &pytania[current_question];
        if (current->used){
            return;
        }
        Player* p = &gracze[fd];
        if (p->ans){
            return;
        }
        p->ans = true;
        std::string proper_anwer_message = "proper\t" + std::string(1, current->poprawna) + "\n";
        this->sendToPlayer(fd, proper_anwer_message);
        current->answers++;
        std::cout << current->answers << gracze.size() << gracze.size() * 2.0 / 3 << current->reduced << std::endl;
        if (current->answers >= (gracze.size() * 2.0 / 3) && !current->reduced){
            current->reduced = true;
            std::string reduced_time_message = "reducedTime\t" + std::to_string(reducedCzas) + "\n";
            this->sendToAllPlayers(reduced_time_message);
            this->sendToHost(reduced_time_message);
        }
        if (answer[0] == current->poprawna){
            p->wynik++;
            if (p->wynik > best_player.wynik) {
                best_player = *p;
            }
        }
    }

    std::string getCode(){
        return code;
    }
};













#define MAX_EVENTS 500

std::unordered_map<int, std::string> nicknames; // fd -> nick
std::unordered_map<int, Quiz*> players; // player_fd -> quiz
std::unordered_map<int, Quiz*> creator; // creator_fd -> quiz
std::vector<Quiz*> quizzes;










std::string generateCode() {
    std::string code;
    auto it = quizzes.end();
    do {
        code = std::to_string(rand() % 90000 + 10000);  // Zakres: 10000 - 99999

        // Sprawdzamy, czy kod już istnieje w quizzes
        auto it = std::find_if(quizzes.begin(), quizzes.end(),
            [&](Quiz* quiz) { return quiz->code == code; });

    } while (it != quizzes.end());  // Jeśli znaleziono, generujemy nowy kod

    return code;
}

Quiz* findQuizByCode(const std::string& game_code) {
    auto it = std::find_if(quizzes.begin(), quizzes.end(),
        [&](Quiz* quiz) { return quiz->code == game_code; });

    return (it != quizzes.end()) ? *it : nullptr;  // Zwracamy quiz lub nullptr, jeśli nie znaleziono
}

void handle_command(int client_fd, std::vector<std::string> command) {
    for (const auto& cmd : command) {
        std::cout << cmd << " ";
    }
    std::cout << std::endl;

    std::cout << nicknames.size() << "\t" << players.size() << "\t" << creator.size() << "\t" << quizzes.size() << std::endl;

    if(command[0] == "nick"){
        // zła komenda
        if( command.size() != 2 || command[1] == ""){
            write(client_fd, "nick\t0\n", 7);
            return;
        }
        std::string nick = command[1];
        // taki sam nick jak poprzednio
        if (nick == nicknames[client_fd]){
            write(client_fd, "nick\t1\n", 7);
            return;
        }
        // taki sam nick jak inny użytownik
        for (const auto& pair : nicknames) {
            if (pair.second == nick) {
                write(client_fd, "nick\t0\n", 7);
                return;
            }
        }
        // poprawny nowy nick
        write(client_fd, "nick\t1\n", 7);        
        nicknames[client_fd] = nick;
        return;
    }
    if(command[0] == "create"){
        // zła komenda
        if (command.size() != 4 || command[1] == "" || command[2] == "" || command[3] == ""){
            write(client_fd, "create\t0\n", 9);
            return;
        }

        std::string nazwa = command[1];
        int czas = std::stoi(command[2]);
        int reducedCzas = std::stoi(command[3]);

        std::string code = generateCode();
        Quiz* q = new Quiz(nazwa, client_fd, nicknames[client_fd], czas, reducedCzas, code);
        creator.insert({client_fd, q});
        quizzes.push_back(q);
        return;
    }
    if(command[0] == "cancel"){
        Quiz* q = creator[client_fd];
        creator.erase(client_fd);
        quizzes.erase(std::remove(quizzes.begin(), quizzes.end(), q), quizzes.end());
        delete q;
        return;
    }
    if(command[0] == "add"){
        // zła komenda
        if (command.size() != 7 || command[1] == "" || command[2] == "" || command[3] == "" || command[4] == "" || command[5] == "" || command[6] == ""){
            return;
        }
        Quiz* q = creator[client_fd];
        Pytanie p;
        p.pytanie = command[1];
        p.odpa = command[2];
        p.odpb = command[3];
        p.odpc = command[4];
        p.odpd = command[5];
        p.poprawna = command[6][0];
        p.answers = 0;
        p.reduced = false;
        p.used = false;
        q->addQuestion(p);
        return;
    }
    if(command[0] == "gameCode"){
        if (creator.find(client_fd) != creator.end()) {
            Quiz* q = creator[client_fd];
            q->empty = false;
            std::string code = q->getCode();
            if (q->inProgress)
            {
                std::string nazwa = q->nazwa;
                int czas = q->czas;
                int reducedCzas = q->reducedCzas;
                Quiz* new_q = new Quiz(nazwa, client_fd, nicknames[client_fd], czas, reducedCzas, generateCode());
                for (const auto& question : q->pytania) {
                    new_q->addQuestion(question);
                }
                new_q->resetQuestions();
                quizzes.push_back(new_q);
                q->noHost = true;
                creator.erase(client_fd);
                creator.insert({client_fd, new_q});
                code = new_q->getCode();
                q->empty = true;
            }
            else{
                q->noHost = false;
                q->empty = false;
            }
            write(client_fd, ("gameCode\t" + code + "\n").c_str(), code.size() + 10);
            q = creator[client_fd];
        } else {
            write(client_fd, "gameCode\t-1\n", 12);
        }
        return;
    }
    if(command[0] == "code"){
        if (command.size() != 2 || command[1] == "") {
            write(client_fd, "code\t0\n", 7);
            return;
        }
        std::string code = command[1];
        for (auto& quiz : quizzes) {
            if (quiz->getCode() == code) {
                if(quiz->inProgress || quiz->noHost){
                    write(client_fd, "code\t0\n", 7);
                    return;
                }
                quiz->addPlayer(client_fd, nicknames[client_fd]);
                players.insert({client_fd, quiz});
                write(client_fd, "code\t1\n", 7);
                return;
            }
        }
        write(client_fd, "code\t0\n", 7);
        return;
    }
    if(command[0] == "gameLobby"){
        std::string game_code = command[1];
        if (players.find(client_fd) != players.end()) {
            Quiz* q = players[client_fd];
            std::string message = "gameLobby\t" + q->nazwa + "\t" + q->creator_nick + "\t" + q->code  + "\n";
            write(client_fd, message.c_str(), message.size());
            for (const auto& player : q->gracze) {
                if (player.first == client_fd)
                {
                    continue;
                }
                std::string message = "player\t" + player.second.nick + "\n";
                write(client_fd, message.c_str(), message.size());
            }           
        } else {
            write(client_fd, "gameLobby\t-1\n", 14);
        }
        return;
    }
    if(command[0] == "noHost"){
        creator[client_fd]->removeHost();
        return;
    }
    if(command[0] == "exit") {
        if (players.find(client_fd) != players.end()) {
            Quiz* q = players[client_fd];
            q->removePlayer(client_fd);
            players.erase(client_fd);
        } 
        else if (creator.find(client_fd) != creator.end()) {
            Quiz* q = creator[client_fd];
            creator.erase(client_fd);
            q->removeHost();
            q->empty = true;
        }
        return;
    }
    if(command[0] == "start") {
        Quiz* q = nullptr;
        try{
            q = creator[client_fd];
        }
        catch(const std::exception& e){
            std::string message = "Player tried to start? Imposible, don't try no tricks on me!\n";
            write(client_fd, message.c_str(), message.size());
            return;
        }
        if (q->inProgress || q->gracze.size() == 0) {
            write(client_fd, "start\t0\n", 8);
            return;
        }
        q->inProgress = true;
        q->resetQuestions();
        q->sendToAllPlayers("start\n");
        q->showQuestion();    
    }
    if(command[0] == "answer") {
        Quiz* q = players[client_fd];
        q->answerQuestion(client_fd, command[1]);
    }
    if(command[0] == "endTime"){
        Quiz* q = players[client_fd];
        Player p = q->gracze[client_fd];
        if(!p.ans){
            q->answerQuestion(client_fd, "0");
        }
    }
    if(command[0] == "scores"){
        if (creator.find(client_fd) != creator.end()) {
            Quiz* q = creator[client_fd];
            for(auto player : q->gracze){
                std::string message = "score\t" + player.second.nick + "\t" + std::to_string(player.second.wynik) + "\n"; 
                q->sendToHost(message);
            }
            
        } else if (players.find(client_fd) != players.end()) {
            Quiz* q = players[client_fd];
            q->pytania[q->current_question].used = true;
            for(auto player : q->gracze){
                if(player.first == client_fd){
                    std::string message = "score\t" + std::to_string(player.second.wynik) + "\n";
                    q->sendToPlayer(client_fd, message);
                }
                else{
                    std::string message = "score\t" + player.second.nick + "\t" + std::to_string(player.second.wynik) + "\n"; 
                    q->sendToPlayer(client_fd, message);
                }
            }
        }
    }
    if(command[0] == "next") {
        if (players.find(client_fd) == players.end()) {
            return;
        }
        Quiz* q = players[client_fd];
        q->gracze [client_fd].ans = false;
        q->nextQuestion();   
    }
}

void handle_read(int client_fd, char* buffer, ssize_t bytes_read) {
    std::vector<std::string> args;
    std::string command = "";
    for(int i = 0; i < bytes_read; i++) {
        if(buffer[i] == '\n') {
            args.push_back(command);
            command = "";
            handle_command(client_fd, args);
            args.clear();
        }
        else if(buffer[i] == '\t'){
            args.push_back(command);
            command = "";
        }
        else {
            command += buffer[i];
        }
    }
}

int main() {
    int server_fd, client_fd, epoll_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    struct epoll_event event, events[MAX_EVENTS];

    // Tworzymy socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return -1;
    }

    int optval = 1;
if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
    perror("setsockopt");
    close(server_fd);
    return -1;
}

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);  // Port serwera
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Wiązanie gniazda
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        return -1;
    }

    // Nasłuchiwanie połączeń
    if (listen(server_fd, 10) == -1) {
        perror("listen");
        return -1;
    }

    // Tworzymy epoll
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return -1;
    }

    // Dodajemy server_fd do epoll
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("epoll_ctl");
        return -1;
    }

    std::cout << "Serwer nasłuchuje na porcie 8080..." << std::endl;

    while (true) {
        quizzes.erase(
            std::remove_if(quizzes.begin(), quizzes.end(),
                [](Quiz* quiz) {
                    if (quiz->empty && quiz->gracze.empty()) {
                        printf("Quiz %s deleted\n", quiz->nazwa.c_str());
                        delete quiz;  // Zwolnienie pamięci
                        return true;  // Usuń wskaźnik z wektora
                    }
                    return false;  // Zachowaj element
                }),
            quizzes.end()
        );
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait");
            return -1;
        }

        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == server_fd) {
                client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                    perror("epoll_ctl");
                    continue;
                }

                nicknames.insert({client_fd, ""});
            } else {
                char buffer[1024];
                ssize_t bytes_read = read(events[i].data.fd, buffer, sizeof(buffer));

                if (bytes_read > 0){
                    handle_read(events[i].data.fd, buffer, bytes_read);
                }
                else if (bytes_read == 0) {
                    //ogranąć rozłączanie
                }
                    
            }
        }
    }

    close(server_fd);
    return 0;
}
