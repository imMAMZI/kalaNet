#ifndef USERS_INFORMATION_WINDOW_H
#define USERS_INFORMATION_WINDOW_H

#include <QMainWindow>

class QLineEdit;
class QTableWidget;
class SessionService;
class UserRepository;

class UsersInformationWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit UsersInformationWindow(UserRepository& userRepository,
                                    SessionService& sessionService,
                                    QWidget* parent = nullptr);

private slots:
    void loadUsers();

private:
    UserRepository& userRepository_;
    SessionService& sessionService_;
    QLineEdit* searchEdit_ = nullptr;
    QTableWidget* table_ = nullptr;
};

#endif // USERS_INFORMATION_WINDOW_H
