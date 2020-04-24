#include<stdio.h> 
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<iostream>
#include<fstream>  
#include<unordered_map>
#include <sys/stat.h>
#include <fcntl.h>
#include<algorithm>	
using namespace std; 

#define SERV_TCP_PORT 5035
#define MAXLINE 4096
#define FD_SETSIZE 15
//Enum
enum Status{mainChoice, username, password, task, animal, animalSound, soundRequired}; 
/*
_______________________________________________________________________________________________________________
        STATUS                             SWITCH CASE/ IF OPTIONS {NextStage / NextStage if invalid}
_______________________________________________________________________________________________________________
|   STAGE1:(mainChoice)    =>     IF: 1(login){2} / 2(register){2}                                         
|   STAGE2:(username)      =>     IF: <input>(mainChoice=1){3/1} / <input>(mainChoice=2){3/1} 
|   STAGE3:(password)      =>     IF: <input>(mainChoice=1){4/1} / <input>(mainChoice=2){4/1} 
|   STAGE4:(task)          =>     SWITCH: store{5a} / sound{5b} / query{4} / bye{-1} / end{-1}    
|   STAGE5b:(soundRequired)=>     IF: stop sound{4} / <input>{5b/5b}
|   STAGE5a:(animal)       =>     <input>{6a}       
|   STAGE6a:(animalSound)  =>     <input>{4}  
________________________________________________________________________________________________________________
*/
class User
{
    public:
	string name;
	string password;

 	User(){
        name = "";
        password = "";
     }
    void add_user(string username, string pass)
    {
		name = username;
		password = pass;			
	}
    
	string getUser_name()
	{
	 	return name;
	}

	string getUser_pass()
	{
	 	return password;
	}
					
};
 

void store_user(User new_user)
{
 	fstream file;
 	char msg[128];
    char * buf;
 	file.open("user.txt",ios::app|ios::binary);
 	if(!file)
 	{
        strcpy(msg, "\nError: Couldn't load required files.\n");
        write(1,msg, strlen(msg));
        exit(0);
    }
    string name = new_user.getUser_name();
    int size1 = name.size();
    string pass = new_user.getUser_pass();
    int size2 = pass.size();

    file.write(reinterpret_cast<char *>(&size1), sizeof(int));
	file.write(name.c_str(), size1);

    file.write(reinterpret_cast<char *>(&size2), sizeof(int));
	file.write(pass.c_str(), size2);

 	file.close();
}	

//User
unordered_map<string, string> users;

//	User database load

bool readData(fstream &file, User &user){

    char *buf;
    string name, pass;
    int size1=0, size2=0;
    if(!file.read(reinterpret_cast<char *>(&size1), sizeof(int)))
        return false;
	buf = new char[size1];
	file.read( buf, size1);
	name = "";
	name.append(buf, size1);

    file.read(reinterpret_cast<char *>(&size2), sizeof(int));
	buf = new char[size2];
	file.read( buf, size2);
	pass = "";
	pass.append(buf, size2);

    user.name = name;
    user.password = pass;
    return true;
}

void load_users()
{
    fstream file;
    User user;
	file.open("user.txt",ios::in|ios::binary);
 	char msg[128];
 		 
 	if(!file)
 	{
        strcpy(msg, "\nError: Couldn't load required files.\n");
        write(1,msg, strlen(msg));
        exit(0);
    }
   
    bool isUser = readData(file, user);
    
	while(isUser)
	{
	    users[user.getUser_name()] = user.password;
        isUser = readData(file, user);
    }
     
   	strcpy(msg,"\nUser database loaded!");
    write(1,msg, strlen(msg));
     
	file.close();
}


//  ANIMAL SOUNDS
unordered_map<string, string> Animals;


string get_sounds(){
    string res = "_______________________________";
    res += "\n\tANIMAL\t|\tSOUND\n";
    res += "_______________________________";
    for(auto animal: Animals){
        res += "\n\t" + animal.first + "\t|\t" + animal.second;
    }
    res += "\n_______________________________";
    return res + "\n";
}

int main()
{

	int                 		i, maxi, maxFd, listenFd, connFd, sockFd;
    int                 		nready;
    int                 		n, choice[FD_SETSIZE];
    fd_set              		rset, allset;
    char                		recv[MAXLINE];
    socklen_t           		clilen;
    struct sockaddr_in  		cliaddr, serv;
    string                      user_names[FD_SETSIZE];
	
    //Client Properties
    struct ClientProperties
    {
        int clientFD, choice;
        enum Status status;
        string username, animal_name;

    } clients[FD_SETSIZE];
/*
____________________________________________________________________________________________________________________
        PROPERTY                             USE 
____________________________________________________________________________________________________________________
|   clientFD    =>   Store ClientFD of each connected client returned from accept 
|   choice      =>   Option choosen by user login(1) /register(2)  
|   status      =>   One of the following {mainChoice, username, password, task, animal, animalSound, soundRequired}
|   username    =>   Username of current client waiting for password    
|   animal_name =>   Animal name entered by client to store and waiting for animal sound
____________________________________________________________________________________________________________________
*/
    Animals["duck"] = "quack";
    Animals["frog"] = "croak";
    Animals["dog"]   = "bark";
    Animals["cat"]   = "meow";
    Animals["lion"]  = "roar";

	listenFd = socket(AF_INET, SOCK_STREAM, 0);
	
	bzero(&serv,sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
	serv.sin_port = htons(SERV_TCP_PORT);

	bind(listenFd,(struct sockaddr *) &serv, sizeof(serv));
	
	write(1,"\nServer in listening state ...\n",30);
	listen(listenFd,10);

	maxFd = listenFd;
	maxi = -1;

    //load users
    load_users();

	for(i = 0; i < FD_SETSIZE; i++) {
		clients[i].clientFD = -1;
        clients[i].choice = -1;
	}
	
	FD_ZERO(&allset);
	FD_SET(listenFd,&allset);	
	
	for(;;){
	
		rset = allset;

		bzero(&recv,sizeof(recv));
        
		nready = select(maxFd + 1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(listenFd, &rset))   //listen ready
		{
			socklen_t len = sizeof(cliaddr);

			connFd = accept(listenFd,(struct sockaddr *) &cliaddr , &len);

			if(connFd == -1) {
				printf("\nNot connected!\n");
				break;
			}
			
			char str[100];

			sprintf(recv,"\nNew client: %s, port %d, connFd %d\n",
                 	inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
                    ntohs(cliaddr.sin_port), connFd);

			write(1,recv,strlen(recv));

			for(i = 0; i < FD_SETSIZE; i++)
				if(clients[i].clientFD < 0) {
					clients[i].clientFD = connFd;
                    clients[i].status = mainChoice;
					break;
				}
				
			if(i == FD_SETSIZE) {
				printf("\nWe are facing huge number of requests. Please try again later.\n");
				return 1;
			}
			
			char reply[MAXLINE] = "\nConnected to ANIMAL SOUND.\nTo continue choose: \n\t 1. REGISTER\n\t 2. LOGIN\n";

			write(connFd, reply, strlen(reply));

			FD_SET(connFd,&allset);
			
			if(connFd > maxFd)
				maxFd = connFd;
			
			if(i > maxi)
				maxi = i;
			
			if(--nready <= 0)
				continue;
				
		}

        //Connected fds
		for(i = 0; i <= maxi; i++) {
			if(clients[i].clientFD < 0)
				continue;
			
			if(FD_ISSET(clients[i].clientFD,&rset)) 
			{
				sockFd = clients[i].clientFD; //clinet fd=sockfd
				
				
				//Client send FIN
				if( (n = read(sockFd, recv, MAXLINE)) == 0)
				{
					char reply[MAXLINE];
					sprintf(reply, "Ending connection\n");
					
					write(sockFd, reply, strlen(reply));
					close(sockFd);
					FD_CLR(sockFd, &allset);
					
					clients[i].clientFD = -1;				
				}
				else 
				{
                    //SWITCH
                    Status current_state = clients[i].status;
                    switch(current_state){

                        case mainChoice:
                        {
                            recv[strlen(recv)-1] = '\0';
                            if(!isdigit(recv[0])){
                                char reply[MAXLINE] = "\nINVALID CHOICE!\n choose: \n\t 1. REGISTER\n\t 2. LOGIN.\n ";
                                write(connFd, reply, strlen(reply));
                                break;
                            }
                            clients[i].choice = stoi(recv);
                            char reply[MAXLINE];
                            if(clients[i].choice == 1)
                            {
                                clients[i].status = username;
                                strcpy(reply,"\n\t\tREGISTER\n_______________________________________\n\nEnter username: ");
                            }
                            else if(clients[i].choice == 2)
                            {
                                clients[i].status = username;
                                strcpy(reply,"\n\t\tLOGIN\n_______________________________________ \n\nEnter username: ");

                            }
                            else{
                                char reply[MAXLINE] = "\nINVALID CHOICE!\n choose: \n\t 1. REGISTER\n\t 2. LOGIN.\n ";
                                write(connFd, reply, strlen(reply));
                                break;
                            }

                            write(sockFd, reply, strlen(reply));
                            break;
                        }

                        case username:
                        {
                            recv[strlen(recv)-1] = '\0';
                            string uname = string(recv);
                            char reply[MAXLINE];

                            if(clients[i].choice == 1){
                                if(users.find(uname)!=users.end()) 
                                {
                                    strcpy(reply,"\nUsername already exist. Try logging in\n\n\t\tMAIN MENU: \n\t 1. REGISTER\n\t 2. LOGIN.\n_______________________________________\n");
                                    clients[i].status = mainChoice;
                                }
                                else 
                                {
                                    strcpy(reply,"\nEnter password: ");
                                    user_names[i] = uname;
                                    clients[i].status = password;
                                }
                                write(sockFd, reply, strlen(reply));		

                            }
                            else if(clients[i].choice == 2)
                            {
                                if(users.find(uname)==users.end()) {
                                    strcpy(reply,"\nUsername does not exist. Register yourself first.\n\n\t\tMAIN MENU: \n\t 1. REGISTER\n\t 2. LOGIN.\n_______________________________________\n ");						
                                    clients[i].status = mainChoice;
                                }

                                else 
                                {
                                    clients[i].status = password;
                                    user_names[i] = uname;
                                    strcpy(reply,"\nEnter password: ");
                                }			

                                write(sockFd,reply,strlen(reply));	
                            }
                            break;
                        }

                        case password:
                        {
                            recv[strlen(recv)-1] = '\0';
                            string userPass = string(recv);
                            char reply[MAXLINE];

                            if(clients[i].choice == 1){

                                users[user_names[i]] = userPass;
                                strcpy(reply,"\nRegistered Successfully !\n");
                                strcat(reply,"\n\n\t\tTASKS\n_______________________________________\n");
                                strcat(reply,"\n 1) QUERY : To check which animal I know");
                                strcat(reply,"\n 2) SOUND : To retrieve sound of a given animal");
                                strcat(reply,"\n 3) STORE : To store (animal, sound) pair");
                                strcat(reply,"\n 4) BYE : To end your current session");
                                strcat(reply,"\n 5) END : To close the server completely");
                                strcat(reply,"\n 6) STOP SOUND : To stop SOUND request \n");
                                strcat(reply,"\n_______________________________________\n");
                                User user;
                                user.add_user(user_names[i],userPass);  //Add registered user user object
                                store_user(user);                       //Store registered user in file
                                clients[i].status = task;
                                write(sockFd, reply, strlen(reply));		
                            }
                            else if(clients[i].choice == 2)
                            {

                                if(users.find(user_names[i])==users.end()) {
                                    strcpy(reply,"\nUsername does not exist. Register yourself first.\n\t\tMAIN MENU: \n\t 1. REGISTER\n\t 2. LOGIN.\n\n_______________________________________\n ");						
                                    clients[i].status = mainChoice;

                                    write(sockFd,reply,strlen(reply));

                                }
                                else 
                                {
                                    if(users[user_names[i]] == userPass) 
                                    {
                                        clients[i].status = task;
                                        strcpy(reply,"\nSuccessfully logged in.\n");
                                        strcat(reply,"\n\n\t\t TASKS: \n_______________________________________\n");
                                        strcat(reply,"\n 1) QUERY : To check which animal I know");
                                        strcat(reply,"\n 2) SOUND : To retrieve sound of a given animal");
                                        strcat(reply,"\n 3) STORE : To store (animal, sound) pair");
                                        strcat(reply,"\n 4) BYE : To end your current session");
                                        strcat(reply,"\n 5) END : To close the server completely");
                                        strcat(reply,"\n 6) STOP SOUND : To stop SOUND request \n");
                                        strcat(reply,"\n_______________________________________\n");
                                    }
                                    else 
                                    { 
                                        clients[i].status = mainChoice;
                                        strcpy(reply,"\nUsername and password does not match. Try again.\n\t\tMAIN MENU: \n\t 1. REGISTER\n\t 2. LOGIN.\n\n_______________________________________\n ");						
                                    }

                                    write(sockFd,reply,strlen(reply));
                                }			

                            }
                            break;
                        }
                        case task:
                        {
                            recv[strlen(recv)-1] = '\0';
                            string request = string(recv);
                            char reply[MAXLINE];

    						transform(request.begin(), request.end(), request.begin(), ::tolower); 	

                            if(request == "sound"){
                                //1 state: soundRequired
                                strcpy(reply, "SOUND OK\n\n");
                                write(sockFd, reply, strlen(reply));
                                clients[i].status = soundRequired;
                                break;
                            }
                            else if(request == "store") {
                                //2 states: animal, animalSound
                                clients[i].status = animal;			
                                strcpy(reply,"\n");
                                write(sockFd, reply, strlen(reply));
                                break;
                            }
                            else if(request == "query"){
                                strcpy(reply, get_sounds().c_str());
                                write(sockFd, reply, strlen(reply));
                                break;
                            }  
                            else if(request == "bye") {
                                strcpy(reply,"\nBYE OK!\n");
                                write(sockFd, reply, strlen(reply));
                                FD_CLR(sockFd, &allset);
                                close(sockFd);
                                clients[i].clientFD = -1;
                                clients[i].status = mainChoice;
                                break;
                            }
                            else if(request == "end") {
                                sprintf(reply, "\nEND OK!");
                                write(sockFd,reply,strlen(reply));
                                sprintf(reply, "Ending server on your request\n");
                                write(1, reply, strlen(reply));
                                exit(0);
                                break;
                            }
                            else
                            {
                            
                                sprintf(reply, "\nINVALID TASK\n");
                                strcat(reply,"\n\n\t\t TASKS: \n_______________________________________\n");
                                strcat(reply,"\n 1) QUERY : To check which animal I know");
                                strcat(reply,"\n 2) SOUND : To retrieve sound of a given animal");
                                strcat(reply,"\n 3) STORE : To store (animal, sound) pair");
                                strcat(reply,"\n 4) BYE : To end your current session");
                                strcat(reply,"\n 5) END : To close the server completely");
                                strcat(reply,"\n 6) STOP SOUND : To stop SOUND request \n");
                                strcat(reply,"\n_______________________________________\n");
                                write(sockFd, reply, strlen(reply));
                                clients[i].status=task;
                                break;
                            } 
                            
                        }
                        case soundRequired:
                        {
                            recv[strlen(recv)-1] = '\0';
                            string request = string(recv);
                            char reply[MAXLINE];

    						transform(request.begin(), request.end(), request.begin(), ::tolower); 

                            if(request == "stop sound"){
                                strcpy(reply,"STOP SOUND OK!\n\n");
                                write(sockFd, reply, strlen(reply));
                                clients[i].status = task ;
                                break;
                            }
                            else if(Animals.find(request) == Animals.end()){
                                strcpy(reply,"Animal Not Found!\nTry another animal or STOP SOUND and STORE this animal\n\n");
                                write(sockFd, reply, strlen(reply));
                                clients[i].status = soundRequired ;
                                break;
                            }
                            else{
                                strcpy(reply,Animals[request].c_str());
                                strcat(reply, "\n\n");
                                clients[i].status = soundRequired ;
                                write(sockFd, reply, strlen(reply));
                                break;
                            }
                           
                        }
                        case animal:
                        {
                            recv[strlen(recv)-1] = '\0';
                            string request = string(recv);
    						transform(request.begin(), request.end(), request.begin(), ::tolower); 
                            clients[i].status = animalSound;
                            clients[i].animal_name = request;
                            break;
                        }
                        case animalSound:
                        {
                            recv[strlen(recv)-1] = '\0';
                            string request = string(recv);
                            char reply[MAXLINE];

    						transform(request.begin(), request.end(), request.begin(), ::tolower); 
                            auto animal_found = Animals.find(clients[i].animal_name);
                            if(animal_found != Animals.end()){
                                Animals.erase(animal_found);
                            }
                            Animals[clients[i].animal_name] = request;
                            strcpy(reply, "\nSTORE OK\n");
                            write(sockFd, reply, strlen(reply));
                            clients[i].status = task;
                            break;
                        }

                    }

                    if (--nready <= 0)
                        break;	
			    }
            }
		}
		
	}	

}
