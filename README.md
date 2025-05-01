**Projects for Operating Systems**:

**Project 1:**

  **Description:**  
  Implements a tabbed web browser controller using GTK+ for the GUI and UNIX IPC via non-blocking pipes to manage communication between
  the controller and rendering processes (tabs). Each browser tab is a seprate child process and the controller handles tab management, URL entry,
  favorites, and tab lifecycle events. 

  **Features:**  
  - GUI-based controller using GTK+  
  - Dynamic tab creation ('+' button)  
  - Independent rendering tabs  
  - Inter-process communication using non-blocking pipes  
  - URL entry handling  
  - Favorties system  
  
  **Run and Compile:**  
  First build everything my using "make", then run make test or run and compile browser.c (ignores blacklist file)
  
  **My Contributions:**  
  browser.c

**Project 2:**  

  **Description:**  
  This project betters project 1 by implementing and improving upon different features such as the tab and process management, the favorites system, 
  Command Handling and IPC, and URL filtering. 

  **Features**  
  - Multiple browser tabs managed through seprate processes each a child forked by the controller
  - Comunication with tabs is done with two pairs of pipes (inbound and outbound)
  - Users can mark URLs as favorites which are saved to '.favorites' and displayed in controller's GUI
  - URLs in '.blacklist' are not able to be loaded in tabs
  - Controller works with GTK's event loop to handle GUI and pipe communication concurrently
  
  **Run and Compile:**  
  First build everything my using "make", then run make test or run and compile browser.c
  
  **My Contributions:**  
  browser.c

**Project 3:**

  **Description:**  
  Implements a multithreaded web server that handles cultiple client requests concurrently using dispatcher and worker threads. It supports queuing, caching,
 logging and content serving. 

 **Features**  
 - Thread pool with configurable number of dispatchers and workers
 - request queue with configurable length
 - File-based caching with FIFO replacement
 - Logging of all client requests
 - Synchronization with pthread mutexes
  
  **Run and Compile:**  
  First build everything my using "make", then run make test or run and compile server.c
  
  **My Contributions:**  
  server.c
