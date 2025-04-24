#include "wrapper.h"
#include "util.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <signal.h>

#define MAX_TABS 100  // this gives us 99 tabs, 0 is reserved for the controller
#define MAX_BAD 1000
#define MAX_URL 100
#define MAX_FAV 100
#define MAX_LABELS 100


comm_channel comm[MAX_TABS];         // Communication pipes
char favorites[MAX_FAV][MAX_URL];    // Maximum char length of a url allowed
int num_fav = 0;                     // # favorites

typedef struct tab_list {
  int free;
  int pid; // may or may not be useful
} tab_list;

// Tab bookkeeping
tab_list TABS[MAX_TABS];
int status = 0;


/************************/
/* Simple tab functions */
/************************/

// return total number of tabs
int get_num_tabs () {
//parses through tabs, counts number of items, and returns
int num = 0;
  for(int i = 0; i < MAX_TABS; i ++ ){
    if (!TABS[i].free){
      num ++;
    }
  }

  return num;
}

// get next free tab index
int get_free_tab () {
//parses through tab array until it finds free tab and returns that tab's index
  for(int i = 1; i < MAX_TABS; i ++ ){
    if (TABS[i].free){
      return i;
    }
  }
//-1 for error
  return -1;
}

// init TABS data structure
void init_tabs () {
  int i;

  for (i=1; i<MAX_TABS; i++)
    TABS[i].free = 1;
  TABS[0].free = 0;
}

/***********************************/
/* Favorite manipulation functions */
/***********************************/

// return 0 if favorite is ok, -1 otherwise
// both max limit, already a favorite (Hint: see util.h) return -1
int fav_ok (char *uri) {
  //checks if already on favorites
  if(on_favorites(uri) == 1){
    perror("url already on favorites");
    return -1;
  }
  //cheks if MAX_FAV is reached
  else if(num_fav >= MAX_FAV){
    perror("MAX_FAV already reached");
    return -1;
  }
  else{
    return 0;
  }
}


// Add uri to favorites file and update favorites array with the new favorite
void update_favorites_file (char *uri) {
  // Add uri to favorites file
  // Update favorites array with the new favorite

  //opens .favorites, prints uri, and closes
  FILE *favFile;
  favFile = fopen(".favorites", "a");
  if(favFile == NULL){
    perror("error opening file");
    exit(-1);
  }
  fprintf(favFile, "%s", uri);
  fclose(favFile);

  //adds uri to array
  //favorites[num_fav] = uri;
  strcpy(favorites[num_fav] ,uri);
  num_fav++;
}

// Set up favorites array
void init_favorites (char *fname) {
  //opens favorites file for reading
  FILE *bfile;
  bfile = fopen(fname, "r");
  if(bfile ==  NULL){
      perror("error with favorites file");
      exit(-1);
  }
  //temp variable to store url
  char temp[MAX_BAD][MAX_URL];
	int i =0;
  while(fgets(temp[i], MAX_URL, bfile)){
  	//copies temp into the blacklist_urls
    // if(fav_ok(temp[i] == -1)){
    //   break;
    // }
		temp[i][strlen(temp[i])-1] = '\0';
  	strncpy(favorites[i], temp[i], MAX_URL);
  	i++;
  }
  num_fav = i;
  fclose(bfile);

  return;
}

// Make fd non-blocking just as in class!
// Return 0 if ok, -1 otherwise
// Really a util but I want you to do it :-)
int non_block_pipe (int fd) {
  int nFlags;
  if((nFlags - fcntl(fd, F_GETFL, 0) < 0)){
    return -1;
  }
  if((fcntl(fd, F_SETFL, nFlags | O_NONBLOCK)) < 0){
    return -1;
  }
  return 0;
}

/***********************************/
/* Functions involving commands    */
/***********************************/

// Checks if tab is bad and url violates constraints; if so, return.
// Otherwise, send NEW_URI_ENTERED command to the tab on inbound pipe
void handle_uri (char *uri, int tab_index) {
  //error checking
  if(bad_format(uri)){
    perror("bad format for url");
    return;
  }
  if(on_blacklist(uri)){
    perror("sorry, url is on blacklist");
  }
  //initializes temp and its fields
  req_t temp;
  temp.type = NEW_URI_ENTERED;
  temp.tab_index = tab_index;
  strcpy(temp.uri, uri);

  //sends the uri entered command to inbound pipe
  if(write(comm[tab_index].inbound[1], &temp, sizeof(req_t)) == -1){
    perror("failed to write to tab");
    return;
  }
  return;

}


// A URI has been typed in, and the associated tab index is determined
// If everything checks out, a NEW_URI_ENTERED command is sent (see Hint)
// Short function
void uri_entered_cb (GtkWidget* entry, gpointer data) {
//error checking
  if(data == NULL) {
    return;
  }

  // Get the tab (hint: wrapper.h)
  int tab = query_tab_id_for_request(entry, data);


  // Get the URL (hint: wrapper.h)
  char *p_url = get_entered_uri(entry);

  // Hint: now you are ready to handle_the_uri
  handle_uri(p_url, tab);


}


// Called when + tab is hit
// Check tab limit ... if ok then do some heavy lifting (see comments)
// Create new tab process with pipes
// Long function
void new_tab_created_cb (GtkButton *button, gpointer data) {
//error check
  if (data == NULL) {
    return;
  }

  // at tab limit?
  if(get_num_tabs()==MAX_TABS){
      return;
  }


  // Get a free tab
  int free = get_free_tab();


  // Create communication pipes for this tab
  pipe(comm[free].inbound);
  pipe(comm[free].outbound);




  // Make the read ends non-blocking
  fcntl(comm[free].inbound[0], F_SETFL, O_NONBLOCK);
  fcntl(comm[free].outbound[0], F_SETFL, O_NONBLOCK);


  // fork and create new render tab
  // Note: render has different arguments now: tab_index, both pairs of pipe fd's
  // (inbound then outbound) -- this last argument will be 4 integers "a b c d"
  // Hint: stringify args
  pid_t pid = fork();

  if(pid == 0){
    //sets the tabNum and pipeFds in preparation for exec
    char tabNum[4];
    char rend[8] = "render";
    snprintf(tabNum,sizeof(tabNum), "%d", free);
    char pipeFds[20];

    sprintf(pipeFds, "%d %d %d %d",
        comm[free].inbound[0],
        comm[free].inbound[1],
        comm[free].outbound[0],
        comm[free].outbound[1]);
//exec call for render
    execl("./render",rend, tabNum, pipeFds, NULL);
  }else{
    TABS[free].free = 0;
    TABS[free].pid = pid;
  }

  // Controller parent just does some TABS bookkeeping
}

// This is called when a favorite is selected for rendering in a tab
// Hint: you will use handle_uri ...
// However: you will need to first add "https://" to the uri so it can be rendered
// as favorites strip this off for a nicer looking menu
// Short
void menu_item_selected_cb (GtkWidget *menu_item, gpointer data) {

  if (data == NULL) {
    return;
  }

  // Note: For simplicity, currently we assume that the label of the menu_item is a valid url
  // get basic uri
  char *basic_uri = (char *)gtk_menu_item_get_label(GTK_MENU_ITEM(menu_item));

  // append "https://" for rendering
  char uri[MAX_URL];
  sprintf(uri, "https://%s", basic_uri);
  //gets tab
  int tab = query_tab_id_for_request(menu_item, data);



  // Hint: now you are ready to handle_the_uri
  handle_uri(uri, tab);

  return;
}


// BIG CHANGE: the controller now runs an loop so it can check all pipes
// Long function
int run_control(){
  browser_window * b_window = NULL;
  int i, nRead;
  req_t req;

  //Create controller window
  create_browser(CONTROLLER_TAB, 0, G_CALLBACK(new_tab_created_cb),
		 G_CALLBACK(uri_entered_cb), &b_window, comm[0]);

  // Create favorites menu
  create_browser_menu(&b_window, &favorites, num_fav);

  while (1) {

    process_single_gtk_event();

    // Read from all tab pipes including private pipe (index 0)
    // to handle commands:
    // PLEASE_DIE (controller should die, self-sent): send PLEASE_DIE to all tabs
    // From any tab:
    //    IS_FAV: add uri to favorite menu (Hint: see wrapper.h), and update .favorites
    //    TAB_IS_DEAD: tab has exited, what should you do?

    //Loop across all pipes from VALID tabs -- starting from 0

    for (i=0; i<MAX_TABS; i++) {
      if (TABS[i].free) continue;

      nRead = read(comm[i].outbound[0], &req, sizeof(req_t));

      // Check that nRead returned something before handling cases

      //Case 1: PLEASE_DIE
      if(nRead != -1 && req.type == PLEASE_DIE){
        for(int j = 1; j < MAX_TABS; j++){
          if(TABS[j].free){
            continue;
          }
          req.type = PLEASE_DIE;
          req.tab_index = j;
          if(write(comm[j].inbound[1], &req, sizeof(req_t)) == -1){
            perror("controller request failed");
          }
        }
        for(int k = 1; k <= get_num_tabs(); k++){
            wait(NULL); 
          }
          exit(0);
      }

      // Case 2: TAB_IS_DEAD
      else if(nRead != -1 && req.type == TAB_IS_DEAD){
        wait(NULL);
        TABS[i].free = 1;
      }

      // Case 3: IS_FAV
      else if(req.type == IS_FAV){
        if(fav_ok(req.uri) == -1){
          alert("something went wrong with adding to favs");
        }
        else{
          add_uri_to_favorite_menu(b_window, req.uri);
          update_favorites_file(req.uri);
        }
      }
    }
    usleep(1000);
  }
  return 0;
}




int main(int argc, char **argv){
//error check for args
  if (argc != 1) {
    fprintf (stderr, "browser <no_args>\n");
    exit (0);
  }
  //initializes blacklist, favorites and tabs
  init_blacklist(".blacklist");
  init_favorites(".favorites");

  init_tabs ();
  // init blacklist (see util.h), and favorites (write this, see above)


  // Fork controller
  // Child creates a pipe for itself comm[0]
  // then calls run_control ()
  // Parent waits ...
  pid_t controller = fork();
  if(controller == 0){
    pipe(comm[0].outbound);
    fcntl(comm[0].outbound[0], F_SETFL, O_NONBLOCK);


    run_control();

  }
  else{
    wait(NULL);
    exit(0);
  }

}