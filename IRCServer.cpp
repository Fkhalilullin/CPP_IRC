#include "IRCServer.hpp"
#include <string>

IRCServer::IRCServer(unsigned int port, std::string pass) :
	_port(port),
	_password(pass),
    _delimeter("\r\n")
{
    char    hostname[30];

    memset(hostname, 0, sizeof(hostname));
    gethostbyname(hostname);
    _hostname = hostname;

    _serverAdress.sin_family = AF_INET;
	_serverAdress.sin_addr.s_addr = INADDR_ANY;
	_serverAdress.sin_port = htons(this->_port);

		// zanulentie fd set 

}

IRCServer::~IRCServer() {}

void IRCServer::_accept()
{
    struct sockaddr_in temp;

    socklen_t socklen = sizeof(struct sockaddr_in); // temp
    int new_fd = accept(_listener, (struct sockaddr*)&temp, &socklen); // reinpretet cast ? // eto new client socket
    if (new_fd == -1)
        throw std::invalid_argument(strerror(errno));
    _addUser(new_fd);
    if (_max_fd < new_fd)
        _max_fd = new_fd;
    FD_SET(new_fd, &_client_fds);
    std::cout << GRE << "New connetion on socket : " << new_fd << END << std::endl;
}

void IRCServer::start()
{
	_listener = socket(AF_INET, SOCK_STREAM, getprotobyname("TCP")->p_proto);
	if (_listener < 0)
		throw std::invalid_argument(strerror(errno));
    _max_fd = _listener;

    fcntl(_listener, F_SETFL, O_NONBLOCK);

    int yes = 1;
    setsockopt(_listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	int b = bind(_listener, (struct sockaddr*)&_serverAdress, sizeof(_serverAdress));
	if (b < 0)
		throw std::invalid_argument(strerror(errno));

    int l = listen(_listener, 10);
	if (l < 0)
		throw std::invalid_argument(strerror(errno));

	FD_SET(_listener, &_client_fds);
	_max_fd = _listener;
	fd_set  select_fds; // just for copy
	select_fds = _client_fds;

    std::cout << GRE << "Server is started..." << END << std::endl;
	while (select(_max_fd + 1, &select_fds, NULL, NULL, NULL) != -1)
    {
		for (int i = 3; i < _max_fd + 1; i++)
        {
			if (!FD_ISSET(i, &select_fds) || i == _listener)
                continue ;
            std::string buf;

            // receiving data
            try
            {
                _recv(i, buf);
            }
            catch (const std::exception& e)
            {
                close(i);
                FD_CLR(i, &this->_client_fds);
                _removeUser(i);
            }
            // command execution
            _execute(i, buf);
            
            std::cout << "Number of users : " 
                        << _users.size() << std::endl;
		}
        if (FD_ISSET(_listener, &select_fds))
            _accept();
		select_fds = _client_fds;
	}
    FD_ZERO(&select_fds );
    FD_ZERO(&_client_fds);
    throw std::invalid_argument(strerror(errno));
}

bool    IRCServer::_recv( int sockfd, std::string &buf ) const
{
    char    c_buf[512];
	int     bytesLeft;
	int     bytes = 1;

    buf.clear();
    while (buf.find(_delimeter) == std::string::npos
                            && buf.length() < sizeof(c_buf))
    {
        memset(c_buf, 0, sizeof(c_buf));
        bytes = recv(sockfd, c_buf, sizeof(c_buf), MSG_PEEK);
        if (bytes < 0)
        {
            std::cerr << RED << strerror(errno) << END;
            throw std::exception();
        }
        if (bytes == 0)
            throw std::exception();

        bytesLeft = std::string(c_buf).find(_delimeter);
        if (bytesLeft == std::string::npos)
            bytesLeft = bytes;
        else
            bytesLeft += _delimeter.length();
        while (bytesLeft > 0)
        {
            memset(c_buf, 0, sizeof(c_buf));
            bytes = recv(sockfd, c_buf, bytesLeft, 0);
            if (bytes < 0)
            {
                std::cerr << RED << strerror(errno) << END;
                throw std::exception();
            }
            if (bytes == 0)
                throw std::exception();
            bytesLeft -= bytes;
            buf       += c_buf;
        }
    }
    std::cout << GRE << "▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽▽" << END << std::endl;
    std::cout << GRE << "-----------RECIEVED-----------" << END << std::endl;
    std::cout << GRE << "socket  : " << END << sockfd << std::endl;
    // std::cout << GRE << "msg len : " << END << buf.length() << std::endl;
    std::cout << GRE << "msg     : " << END << buf << std::endl;
    std::cout << GRE << "△△△△△△△△△△△△△△△△△△△△△△△△△△△△△△" << END << std::endl;
    buf.erase(buf.end() - _delimeter.length(), buf.end());
    return (bytes == -1 ? false : true);
}

bool	IRCServer::_send( int sockfd, const std::string &buf ) const
{
    std::string buf_delim(buf);
    int         total = 0;
    int         bytesLeft;
    int     	bytes;

    if (buf.empty())
        return (false);
    if (buf_delim.find(_delimeter) != buf_delim.length() - _delimeter.length())
        buf_delim += _delimeter;
    bytesLeft = buf_delim.length();
    while (bytesLeft > 0)
    {
        bytes = send(sockfd, buf_delim.c_str() + total, bytesLeft, 0);
        if (bytes < 0)
        {
            std::cerr << RED << strerror(errno) << END;
            break ;
        }
        total += bytes;
        bytesLeft -= bytes;
    }
    std::cout << YEL << "▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼" << END << std::endl;
    std::cout << YEL << "------------SENDED------------" << END << std::endl;
    std::cout << YEL << "socket  : " << END << sockfd << std::endl;
    // std::cout << YEL << "msg len : " << END << buf_delim.length() << std::endl;
    std::cout << YEL << "msg     : " << END << buf_delim << std::endl;
    std::cout << YEL << "▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲" << END << std::endl;
    return (bytes == -1 ? false : true);
}

void    IRCServer::_removeUser(int sockfd)
{
    std::multimap<std::string, User>::iterator  it;

    it = _users.begin();
    while (it != _users.end() && it->second.getSocket() != sockfd)
        it++;
    if (it != _users.end())
        _users.erase(it);
}

void    IRCServer::_addUser(int sockfd)
{
    User new_user;

    new_user.setSocket(sockfd);
    std::pair<std::string, User> tmp(std::string(""), new_user);
    _users.insert(tmp);
}

void    IRCServer::_addUser(const User &user)
{
    std::pair<std::string, User> tmp(user.getNickname(), user);
    _users.insert(tmp);
}

void    IRCServer::_execute( int sockfd, const std::string &buf )
{
    std::multimap<std::string, User>::iterator  it;
            
    it = _users.begin();
    while (it != _users.end() && it->second.getSocket() != sockfd)
        it++;
    if (it == _users.end())
        return ; // no such user
    
    Message msg(buf, it->second);
    User    *user = &it->second;

    _CAP (msg, *user);
    _PASS(msg, *user);
    _PING(msg, *user);
    if (user->isPassworded())
    {
        _NICK(msg, &user);
        _USER(msg, *user);
    }
    if (user->isPassworded() && user->isLogged())
    {
        _PRIVMSG(msg, *user);
        _JOIN   (msg, *user);
        _LIST   (msg, *user);
        _OPER   (msg, *user);
        if (utils::toUpper(msg.getCommand()) == "NAMES")
            _NAMES  (msg, *user);
        _TOPIC  (msg, *user);
        _INVITE (msg, *user);
    }
}

void IRCServer::_PRIVMSG(const Message &msg, const User &usr) {
	std::multimap<std::string, User>::iterator us_it;
	std::map<std::string, Channel>::iterator ch_it;
	std::string buf;

    if (utils::toUpper(msg.getCommand()) != "PRIVMSG")
        return ;

    if (msg.getParamets().empty()) {
        buf = ":" + this->_hostname 
                  + " 411 " 
                  + usr.getNickname() + 
                  + " :No recipient given PRIVMSG";
        _send(usr.getSocket(), buf);
        return ; 
    }

    if(msg.getParamets().size() == 1) {
        buf = ":" + this->_hostname 
                  + " 412 " 
                  + usr.getNickname() 
                  + " :No text to send";
        _send(usr.getSocket(), buf);
        return ; 
    }

    for (int i = 0; i != msg.getParamets().size() - 1; ++i) {
        for (int j = 0; j != msg.getParamets().size() - 1; ++j) {
            if (i != j && msg.getParamets()[i] == msg.getParamets()[j]) {
                buf = ":" + this->_hostname 
                          + " 407 "
                          + usr.getNickname() 
                          + " " 
                          +  msg.getParamets()[i] 
                          + " :Duplicate recipients. No message delivered";
                us_it = this->_users.find(msg.getParamets()[i]);
                _send(us_it->second.getSocket(), buf);
                return ;
            }
        }
    }

	us_it = this->_users.begin();
    ch_it = this->_channels.begin();
    for (int i = 0; i != msg.getParamets().size() - 1; ++i) {
	    us_it = this->_users.find(msg.getParamets()[i]);
        ch_it = this->_channels.find(msg.getParamets()[i]);
	    if (us_it != this->_users.end()) {
            std::string message(":" + msg.getPrefix() 
                                    + " PRIVMSG " 
                                    + us_it->second.getNickname() 
                                    + " :" 
                                    + msg.getParamets().back()); 

		    _send(us_it->second.getSocket(), message);
        }
        else if (ch_it != this->_channels.end()) {
            std::string message(":" + msg.getPrefix() 
                                    + " PRIVMSG " 
                                    + ch_it->second.getName() 
                                    + " :" 
                                    + msg.getParamets().back()); 
            std::map<std::string, User*>::const_iterator us_ch_it;

            us_ch_it = ch_it->second.getUsers().begin();
            for (;us_ch_it != ch_it->second.getUsers().end(); ++us_ch_it) {
		        _send(us_ch_it->second->getSocket(), message);
            }
        }
        else{
            buf = ":" + this->_hostname 
                      + " 401 " 
                      + usr.getNickname() 
                      + " " 
                      + msg.getParamets()[i] 
                      + " :No such nick/channel";
            _send(usr.getSocket(), buf);
            return ; 
        }
	}
}

void    IRCServer::_CAP( const Message &msg, const User &user ) const
{
    std::string	buf;

    if (utils::toUpper(msg.getCommand()) != "CAP")
        return ;
    if (msg.getParamets().size() > 0 && msg.getParamets()[0] == "LS")
    {
        buf = "CAP * LS :";
        _send(user.getSocket(), buf);
    }
}

void    IRCServer::_PASS( const Message &msg, User &user )
{
    std::string	buf;

    if (utils::toUpper(msg.getCommand()) != "PASS")
        return ;
    if (user.isPassworded())
    {
        buf = "462 :You may not reregister";
        _send(user.getSocket(), buf);
        return ;
    }
    if (msg.getParamets().size() == 0)
    {
        buf = "461 PASS :Not enough parameters";
        _send(user.getSocket(), buf);
        return ;
    }
    if (msg.getParamets()[0] == _password)
        user.switchPassword();
    else
    {
        if (user.getNickname().empty())
            buf = "464 * :Password incorrect";
        else
            buf = "464 " + user.getNickname() + " :Password incorrect";
        _send(user.getSocket(), buf);
    }
}

bool    IRCServer::_isCorrectNick( const std::string &nick )
{
    if (nick.length() > NICKLEN || nick.length() == 0)
        return (false);
    if (!((nick[0] >= 'a' && nick[0] <= 'z') || (nick[0] >= 'A' && nick[0] <= 'Z')))
        return (false);
    for (size_t i = 1; i < nick.length(); ++i)
    {
        if (!(     (nick[i] >= 'a' && nick[i] <= 'z')
                || (nick[i] >= 'A' && nick[i] <= 'Z')
                || (nick[i] >= '0' && nick[i] <= '9')
                || nick[i] == '-'
                || nick[i] == '['
                || nick[i] == ']'
                || nick[i] == '\\'
                || nick[i] == '\''
                || nick[i] == '^'
                || nick[i] == '{'
                || nick[i] == '}'
            ))
            return (false);
    }
    return (true);
}

void    IRCServer::_NICK( const Message &msg, User **user )
{
    std::string	buf;

    if (utils::toUpper(msg.getCommand()) != "NICK")
        return ;
    if (msg.getParamets().size() == 0)
    {
        buf = "431 :No nickname given";
        _send((*user)->getSocket(), buf);
        return ;
    }
    if (!_isCorrectNick(msg.getParamets()[0]))
    {
        buf = "432 " + msg.getParamets()[0] + " :Erroneus nickname";
        _send((*user)->getSocket(), buf);
        return ;
    }
    if (_users.find(msg.getParamets()[0]) != _users.end())
    {
        buf = "433 " + msg.getParamets()[0] + " :Nickname is already in use";
        _send((*user)->getSocket(), buf);
        return ;
    }
    std::string oldNick((*user)->getNickname());
    User        copy(**user);

    copy.setNickname(msg.getParamets()[0]);
    _removeUser((*user)->getSocket());
    _addUser(copy);
    if (oldNick.empty())
        buf = "NICK " + copy.getNickname();
    else
        buf = ":" + oldNick + " NICK :" + copy.getNickname();
    (*user) = &(_users.find(copy.getNickname())->second);
    _send((*user)->getSocket(), buf);
}

void    IRCServer::_USER( const Message &msg, User &user )
{
    std::string	buf;

    if (utils::toUpper(msg.getCommand()) != "USER")
        return ;
    if (msg.getParamets().size() < 4)
    {
        buf = "461 NICK :Not enough parameters";
        _send(user.getSocket(), buf);
        return ;
    }
    if (user.isLogged())
    {
        buf = "462 :Already registered";
        _send(user.getSocket(), buf);
        return ;
    }
    user.switchLogged();
    buf = "001 "
        + user.getNickname()
        + " :Welcome to the Internet Relay Network, "
        + user.getNickname()
        + "\r\n";
    buf += "002 "
        + user.getNickname()
        + " :Your host is <servername>, running version <version>"
        + "\r\n";
    buf += "003 "
        + user.getNickname()
        + " :This server was created <datetime>"
        + "\r\n";
    buf += "004 "
        + user.getNickname()
        + " <servername> 1.0/UTF-8 aboOirswx abcehiIklmnoOpqrstvz"
        + "\r\n";
    buf += "005 "
        + user.getNickname()
        + " PREFIX=(ohv)@\%+ CODEPAGES MODES=3 CHANTYPES=#&!+ MAXCHANNELS=20 NICKLEN=31 TOPICLEN=255 KICKLEN=255 NETWORK=school21 CHANMODES=beI,k,l,acimnpqrstz :are supported by this server"
        + "\r\n";
    _send(user.getSocket(), buf);
}

void    IRCServer::_PING( const Message &msg, const User &user ) const
{
    std::string	buf;

    if (utils::toUpper(msg.getCommand()) != "PING")
        return ;
    buf = "PONG " + _hostname;
    _send(user.getSocket(), buf);
}

void IRCServer::_NOTICE(const Message &msg, const User &usr) {
	std::multimap<std::string, User>::iterator us_it;
    std::map<std::string, Channel>::iterator ch_it;
    std::string buf;

    if (utils::toUpper(msg.getCommand()) != "NOTICE")
        return ;

    if (msg.getParamets().empty()) {
        return ; 
    }

    if(msg.getParamets().size() == 1) {
        return ; 
    }

    for (int i = 0; i != msg.getParamets().size() - 1; ++i) {
        for (int j = 0; j != msg.getParamets().size() - 1; ++j) {
            if (i != j && msg.getParamets()[i] == msg.getParamets()[j]) 
                return ;
        }
    }

	us_it = this->_users.begin();
    ch_it = this->_channels.begin();
    for (int i = 0; i != msg.getParamets().size() - 1; ++i) {
	    us_it = this->_users.find(msg.getParamets()[i]);
        ch_it = this->_channels.find(msg.getParamets()[i]);
	    if (us_it != this->_users.end()) {
            std::string message(":" + msg.getPrefix() 
                                    + " PRIVMSG " 
                                    + us_it->second.getNickname() 
                                    + " :" 
                                    + msg.getParamets().back()); 

		    _send(us_it->second.getSocket(), message);
        }
        else if (ch_it != this->_channels.end()) {
            std::string message(":" + msg.getPrefix() 
                                    + " PRIVMSG " 
                                    + ch_it->second.getName() 
                                    + " :" 
                                    + msg.getParamets().back()); 
            std::map<std::string, User*>::const_iterator us_ch_it;

            us_ch_it = ch_it->second.getUsers().begin();
            for (;us_ch_it != ch_it->second.getUsers().end(); ++us_ch_it) {
		        _send(us_ch_it->second->getSocket(), message);
            }
        }
        else
            return ; 
	}
}

void IRCServer::_JOIN(const Message &msg, User &usr) {
	if (utils::toUpper(msg.getCommand()) != "JOIN")
        return ;

    if (msg.getParamets().empty()) {
		std::cout << "461 " << ":Not enough parameters" << std::endl;
		std::string to_send = "461 JOIN :Not enough parameters\r\n";
		// "<client> <command> :Not enough parameters"
		_send(usr.getSocket(), to_send);
		return;
    }

	if (msg.getParamets()[0][0] != '#' && msg.getParamets()[0][0] != '&') {
		// no such err for user
		std::cout << "first param should be group" << std::endl;
		return;
	}

	std::vector<std::string> params;
	std::vector<std::string> passwords;
	for (int i = 0; i < msg.getParamets().size(); i++) {
		
		std::string tmp_param = msg.getParamets()[i];
		for (int k = 0; k < tmp_param.size(); k++) { // check valkd name
			if (tmp_param[k] == ' ' ||
				tmp_param[k] == ',' ||
				tmp_param[k] == '\a' ||
				tmp_param[k] == '\0' ||
				tmp_param[k] == '\r' ||
				tmp_param[k] == '\n') {
					// no err for user
					std::cout << "invalid name of group" << std::endl; // send to serv
					return ;
			}
		}
		
		if (tmp_param[0] == '#' || tmp_param[0] == '&') {
			tmp_param.erase(0, 1);
			params.push_back(tmp_param);
		}
		else {
			passwords.push_back(tmp_param);
		}
	}

	if (passwords.size() > params.size()) {
		// no err for user
		std::cout << "more passes than groups" << std::endl;
		return;
	}

	for (int i = 0; i < params.size(); i++) {
		std::string tmp_group = params[i];

		std::map<std::string, Channel>::iterator ch_it;
		ch_it = this->_channels.find(tmp_group);

		if (ch_it != this->_channels.end()) { // finded group

		std::cout << YEL << "group exists\n" << END;

        std::cout << "PASS BEFORE CHECKING " << ch_it->second._pass << "    " << std::endl;
        
        
        try { // maybe incorrect
            passwords.at(i);
            if (ch_it->second._pass == passwords[i])
                std::cout << GRE << "pass is ok" << END << std::endl;
            else {
                std::cout << RED << "pass not ok" << END << std::endl;
                return;
            }
        }
        catch( ... ) {
            if (!ch_it->second._pass.empty()) {
                std::cout << RED << "group has pass but i didnt get it" << END << std::endl;
                return;
            }
            else {
                std::cout << GRE << "gropu hasnt pass welcome" << END << std::endl;
            }
        }

		std::cout << YEL << "after pass\n" << END;

            std::map<std::string, User*>::const_iterator user_search_it;
			user_search_it = ch_it->second.getUsers().find(usr.getNickname());
			if (user_search_it != ch_it->second.getUsers().end()) {
				std::cout <<  GRE << "user already in group" << END << std::endl;
				// the same info to user
				return;
			}

	// 		// 1. check mode of channnel MODE	

			// checking if user doesnt banned on channel
			for (int i = 0; i < ch_it->second._ban_list.size(); i++) {
				if (ch_it->second._ban_list[i] == usr.getNickname()) {
					std::cout << "user banned on channel" << std::endl;
					// add msg to user
					return;
				}
			}

			int num_groups_user_in = 0;
			std::map<std::string, Channel>::iterator max_group_it;
			max_group_it = this->_channels.begin();
			for (; max_group_it != this->_channels.end(); max_group_it++) {
			if (max_group_it->second.getUsers().find(usr.getNickname()) != max_group_it->second.getUsers().end())
				num_groups_user_in++;
			}
			if (num_groups_user_in > 10) {
				std::cout << "more than 10 groups" << std::endl;
				// msg to usr about it
			// (405 ERR_TOOMANYCHANNELS)			
				return;
			}

			ch_it->second.addUser(usr);
		
	// 		// if joined user -> send msg about new user to all
	// 			// _PRIVMSG TO ALL USER IN GROUP

	// 		// and Если JOIN прошла хорошо, пользователь получает топик канала + СПИСОК ЛЮДЕЙ
	// 		std::string welcome_str = "welcome to " + ch_it->second.getName() + " group\n";
	// 		welcome_str += "the topic of the channel is " + ch_it->second.getTopic(); 
	// 		this->_send(usr.getSocket(), welcome_str);
		}
		else { // second var
	// 		// count num of groups in
			int num_groups_user_in = 0;
			std::map<std::string, Channel>::iterator ban_it;
			ban_it = this->_channels.begin();
			for (; ban_it != this->_channels.end(); ban_it++) {
			if (ban_it->second.getUsers().find(usr.getNickname()) != ban_it->second.getUsers().end())
				// "474 <client> <channel> :Cannot join channel (+b)"
				num_groups_user_in++;
			}
			if (num_groups_user_in > 10) {
				// msg to usr about it
				// "405 <client> <channel> :You have joined too many channels"
				std::cout << "more than 10 groups" << std::endl;
				return;
			}

			// std::cout << GRE << "group creating..." << END << std::endl;
			// if (usr._nickname == "") { // is.empty
			// 	std::cout << "group should have name of owner" << std::endl;
			// 	return;
			// }

			Channel new_ch(tmp_group);

			new_ch.addUser(usr);
			new_ch.addChop(usr);
			
			try {
				passwords.at(i);
                std::cout << "prisvoim " << passwords[i]<< std::endl; 
				new_ch._pass = passwords[i];
			}
			catch ( ... ) {}
            std::cout << "in else pass " << new_ch._pass << std::endl; 

	// 		// info to server that usr is chop now to server
			std::string to_send = "now you an admin of " + new_ch.getName() + " group";
			this->_send(usr.getSocket(), to_send);

            // after filling all params -> add to group list
			this->_channels.insert(std::make_pair(new_ch.getName(), new_ch));
		}
	}

	// std::cout << "\n\nAFTER LOOP " << std::endl;
	// std::map<std::string, Channel>::iterator check; // del
	// for (check = _channels.begin(); check != _channels.end(); check++)
	// 	std::cout << RED << "proverka group after " << check->first << std::endl;

}

void IRCServer::_PART(const Message &msg, const User &usr) {
	// dolzhno rabotat' s neskol'kimi params(groups)!!
	// Параметры: <channel>{,<channel>}
	// Сообщение PART удаляет клиента, пославшего эту команду из списка
	// активных пользователей для всех каналов, указанных в параметре.
	// PART #twilight_zone // # is ok?

	// loop po vsem channels -> delete user//
    if (utils::toUpper(msg.getCommand()) != "PART")
        return ;

	std::map<std::string, Channel>::iterator ch_it;
	ch_it = this->_channels.find(msg.getParamets()[0]); // najti group
	if (ch_it != this->_channels.end()) {
		// gruppa finded
		ch_it->second.removeUser(usr._nickname); // add getter getNick
		// msg to all sockets that user left channel
	}
	else {
		// group not finded -> return ERR
	}
}

void    IRCServer::_OPER( const Message &msg, const User &user )
{
    std::string                                 buf;
    std::multimap<std::string, User>::iterator  it;

    if (utils::toUpper(msg.getCommand()) != "OPER")
        return ;
    if (msg.getParamets().size() < 2)
        buf = ":" + _hostname + " 461 " + user.getNickname() + " OPER :Not enough parameters";
    else if (msg.getParamets()[0] != user.getNickname())
        buf = ":" + _hostname + " 491 " + user.getNickname() + " :No O-lines for your host";
    else if (msg.getParamets()[1] != _password)
        buf = ":" + _hostname + " 464 " + user.getNickname() + " :Password incorrect";
    else
    {
        it = _users.find(user.getNickname());
        if (it != _users.end())
        {
            _operators.insert(std::make_pair(user.getNickname(), &it->second));
            buf = ":" + _hostname + " 381 " + user.getNickname() + " :You are now an IRC operator";
        }
        else
            std::cerr << RED << "Something went wrong : OPER : No such user" << END << std::endl;
    }
    _send(user.getSocket(), buf);
}

void IRCServer::_LIST( const Message &msg, const User &user ) const
{
    std::map<std::string, Channel>::const_iterator  it;
    std::string                                     buf;

    if (utils::toUpper(msg.getCommand()) != "LIST")
        return ;
    buf = ":" + _hostname + " 321 " + user.getNickname() + " Channel :Users  Name";
    _send(user.getSocket(), buf);
    if (msg.getParamets().size() == 0)
    {
        for (it = _channels.begin(); it != _channels.end(); ++it)
        {
            const Channel       &channel = it->second;
            std::stringstream   ss;

            ss << channel.getUsers().size();
            buf = ":"
                + _hostname
                + " 322 "
                + user.getNickname()    + " "
                + channel.getName()     + " "
                + ss.str()              + " :"
                + channel.getTopic();
            _send(user.getSocket(), buf);
        }
    }
    else
    {
        for (size_t i = 0; i < msg.getParamets().size(); ++i)
        {
            it = _channels.find(msg.getParamets()[i]);
            
            if (it == _channels.end())
                continue ;

            const Channel       &channel = it->second;
            std::stringstream   ss;

            ss << channel.getUsers().size();
            buf = ":"
                + _hostname
                + " 322 "
                + user.getNickname()    + " "
                + channel.getName()     + " "
                + ss.str()              + " :"
                + channel.getTopic();
            _send(user.getSocket(), buf);
        }
    }
    buf = ":" + _hostname + " 323 " + user.getNickname() + " :End of /LIST";
    _send(user.getSocket(), buf);
}

void    IRCServer::_TOPIC(const Message &msg, const User &user) {
    std::map<std::string, Channel>::iterator ch_it;

    std::string buf_string; 
    std::string buf;

    if (utils::toUpper(msg.getCommand()) != "TOPIC")
        return ;

    if (msg.getParamets().size() == 0) {
        buf = ":" + this->_hostname  
                  + " 461 " 
                  + user.getNickname() 
                  + " " 
                  + "TOPIC :Not enough parameters";
        _send(user.getSocket(), buf);
        return ;
    }

    buf_string = msg.getParamets()[0];
    if (buf_string[0] == '#') {
        buf_string.erase(0,1);
    }
    else {
        buf = ":" + this->_hostname  
                  + " 403 " 
                  + user.getNickname() 
                  + " " + buf_string 
                  +  " :No such channel";
        _send(user.getSocket(), buf);
        return ;
    }
    
    ch_it = this->_channels.find(buf_string);

    if (ch_it != _channels.end()) {
        if (msg.getParamets().size() == 2) {
            ch_it->second.setTopic(msg.getParamets()[1]);

            buf = ":" + user.getNickname()
                      + " TOPIC" + " #" 
                      + buf_string 
                      + " :" 
                      + ch_it->second.getTopic();
            
            _send(user.getSocket(), buf);
            return ;    
        }
        else if (ch_it->second.getTopic().empty()) {
            buf = ":" + this->_hostname  
                  + " 331 " 
                  + user.getNickname() 
                  + " #" 
                  + buf_string 
                  + " :No topic is set";
            _send(user.getSocket(), buf);
            return ;    
        }
        else if (!ch_it->second.getTopic().empty()) {
            buf = ":" + this->_hostname  
                  + " 332 " 
                  + user.getNickname() 
                  + " #" 
                  + buf_string 
                  + " :"
                  + ch_it->second.getTopic();
             _send(user.getSocket(), buf);
        }
    }
    else {
        buf =  ":" + this->_hostname  
                  + " 403 " 
                  + user.getNickname() 
                  + " #" 
                  + buf_string 
                  + " :No such channel";
        _send(user.getSocket(), buf);
        return ;
    }
}

void IRCServer::_NAMES(const Message &msg, const User &user) {

    std::map<std::string, Channel>::iterator ch_it;
	std::string message;
	std::string buf;
    std::vector<std::string> buf_string;

    // if (utils::toUpper(msg.getCommand()) != "NAMES")
    //     return ;
	
	// if (_channels.empty())
	// 	return ;

    for (int i = 0; i < msg.getParamets().size(); ++i) {
        buf_string.push_back(msg.getParamets()[i]);
        if (!buf_string.empty() && buf_string[i][0] == '#') {
            buf_string[i].erase(0, 1);
        }
        else
            buf_string.pop_back();
    }


	ch_it = _channels.begin();
	if (! buf_string.empty()) {
		for (int i = 0; i <  buf_string.size(); ++i) {
			ch_it = this->_channels.find( buf_string[i]);
			if (ch_it != _channels.end()) {
				std::map<std::string, User*>::const_iterator ch_us_it; 
				std::map<std::string, User*>::const_iterator ch_chops_it;
				ch_us_it = ch_it->second.getUsers().begin();
				message =":" + this->_hostname 
                             + " 353 " + user.getNickname() 
							 + " = #"
							 + ch_it->second.getName() 
						     + " :";
				for (; ch_us_it != ch_it->second.getUsers().end(); ++ch_us_it) {
					ch_chops_it = ch_it->second.getChops().find(ch_us_it->second->getNickname());
					if (ch_chops_it != ch_it->second.getChops().end())
						buf += "@" + ch_us_it->second->getNickname() + " ";
					else
						buf += ch_us_it->second->getNickname() + " ";
				}
				_send(user.getSocket(), message + buf);
                message = ":" + this->_hostname 
                              + " 366 "  + user.getNickname() 
                              + " #"
                              + ch_it->second.getName() 
                              + " :End of /NAMES list";
                _send(user.getSocket(), message);
			} 
            else {
                message = ":" + this->_hostname 
                              + " 366 "  + user.getNickname() 
                              + " #"
                              + buf_string[i]
                              + " :End of /NAMES list";
                _send(user.getSocket(), message);
            }
		}
	}
}

void IRCServer::_KICK(const Message &msg, const User &usr) {
	// Команда: KICK
	// Параметры: <channel> <user> [<comment>]

	std::string str_channel = msg.getParamets()[0]; // true??
	std::string str_user = msg.getParamets()[1]; // true??

    if (utils::toUpper(msg.getCommand()) != "KICK")
        return ;

	// channel exists?
	std::map<std::string, Channel>::iterator ch_it;
	ch_it = this->_channels.begin();
	// if (ch_it != this->_channels.end())
		//
	// if (ch_it->second._users.find(str_user))
		// ch_it->second.removeUser(str_user);
		// will socket of user deleted?
	

	// else
		// no channel
}

void IRCServer::_INVITE (const Message &msg, const User &user) {

	// Команда: INVITE
	// Параметры: <nickname> <channel>
    std::string buf;
    std::string buf_string;

    if (utils::toUpper(msg.getCommand()) != "INVITE")
        return ;
    
    if (msg.getParamets().size() < 2) {
        buf = ":" + this->_hostname  
                  + " 461 " 
                  + user.getNickname() 
                  + " " 
                  + "TOPIC :Not enough parameters";
        _send(user.getSocket(), buf);
        return ;
    }

    buf_string = msg.getParamets()[1];
    if (buf_string[0] == '#') {
        buf_string.erase(0,1);
    }
    else {
        return ;
    }
    
    
}
