#ifndef IRCSERVER_HPP
#define IRCSERVER_HPP

#include "utils.hpp"

#include <sys/select.h>

#include <map>
#include <string>
// #include <string.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <exception>
#include <sys/time.h>


#include "User.hpp"
#include "Channel.hpp"
#include "Message.hpp"

// clean it
#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>	//close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO
#include <netdb.h> // proto
#include <iostream>
#include <sstream>
#include <vector>
#include <ctime>

#define RED "\033[31m"
#define GRE "\033[32m"
#define YEL "\033[33m"
#define BLU "\033[34m"
#define END "\033[37m"

#define MAXCHANNELS 20
#define NICKLEN     9
#define TOPICLEN    255

class User;
class Channel;
class Message;

class IRCServer
{
    private:
        int                              _listener;
        int                              _max_fd; // ne bolshe 10
        unsigned int                     _port;
        fd_set                           _client_fds;
        struct sockaddr_in               _serverAdress;
        std::string                      _hostname;
        std::string                      _password;
        std::multimap<std::string, User> _users;
        std::map<std::string, User*>     _operators;
        std::map<std::string, Channel>   _channels;
        std::string                      _delimeter;

    public:
        explicit IRCServer( unsigned int port, std::string pass );
        ~IRCServer();
        void        start();
        // void        stop ();

    private:
        void    _accept    ();
        bool    _recv      ( int sockfd,       std::string &buf ) const;
        bool    _send      ( int sockfd, const std::string &buf ) const;
        void    _exec      ( const Message &msg );
        void    _addUser   ( int sockfd         );
        void    _addUser   ( const User &user   );
        void    _removeUser( int sockfd         );
        bool    _isCorrectNick( const std::string &nick );

        // Begin CMD

		void    _execute( int sockfd, const std::string &buf );
        void    _PRIVMSG( const Message &msg, const User &usr);
        void    _CAP    ( const Message &msg, const User &user ) const;
        void    _PASS   ( const Message &msg, User &user );
        void    _NICK   ( const Message &msg, User &user );
        void    _USER   ( const Message &msg, User &user );

		void    _PING   ( const Message &msg, const User &user ) const;

		void    _NOTICE (const Message &msg, const User &usr); // k
		void    _JOIN   (const Message &msg, User &usr); // bez const	// k
		void    _PART   (const Message &msg, const User &usr); // k
		void    _OPER   (const Message &msg); // k
		void    _LIST   (const Message &msg, const User &user); //const;
		void    _NAMES  (const Message &msg, const User &user); // k

        // QUIT cmd -? -> JUST exit from server?

        void _KICK(const Message &msg, const User &usr);
        void _INVITE(const Message &msg);
};

#endif
