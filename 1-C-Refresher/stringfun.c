#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define BUFFER_SZ 50

//prototypes
void usage(char *);
void print_buff(char *, int);
int  setup_buff(char *, char *, int);

//prototypes for functions to handle required functionality
int  count_words(char *, int, int);
//add additional prototypes herejj


int setup_buff(char *buff, char *user_str, int len){
    //TODO: #4:  Implement the setup buff as per the directions
    //return 0; //for now just so the code compiles.
	 char *src = user_str; // pointer to traverse user_str
	 char *dst = buff; // Pointer to populate buff
	 int char_count = 0; //Number of characters in user_str
	 int consecutive_space = 0;
	 int has_valid_word = 0;

	 // skip leading whitespace
	 while (*src ==' ' || *src =='\t'){
	 src++;
	 }
	 // Traverse user_str
	 while (*src != '\0'){
	 if (*src == ' ' || *src == '\t'){
	 	if(!consecutive_space && char_count < len-1){
			*dst++ = ' '; // add single space
			char_count++;
			consecutive_space = 1;
			}
	 	}else{
			if (char_count < len -1){
				*dst++ = *src; // add non-whitespace character
				char_count++;
				consecutive_space = 0;
				has_valid_word = 1;
			}else{
				return -1; // Error; usr_str too long
			}
		}
			src++;
			}


	//remove trailing white space
	if (char_count > 0 && *(dst-1)== ' '){
		dst--;
		char_count--;
	}

	// fill remaining buffer spcae with '.'
	while (char_count < len) {
		*dst++ = '.';
		char_count++;
	}

	return has_valid_word ? char_count : 0 ; //return the length of user_str
}

void print_buff(char *buff, int len){
    printf("Buffer:  ");
    for (int i=0; i<len; i++){
        putchar(*(buff+i));
    }
    putchar('\n');
}

void usage(char *exename){
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);

}

int count_words(char *buff, int len, int str_len){
    //YOU MUST IMPLEMENT
    //return 0;
	 int word_count = 0;
	 int in_word = 0;

	 if (str_len == 0 || (str_len == 1 && buff[0] == ' ')){
		return 0;
	 }
	 for(int i = 0; i < str_len; i++){
		if (buff[i] != ' '){
			if (!in_word){
				word_count++; // New word starts
				in_word = 1;
			}
		} else{
			in_word = 0; // reset for spaces
		}	
	 }
	 return word_count;
}

//ADD OTHER HELPER FUNCTIONS HERE FOR OTHER REQUIRED PROGRAM OPTIONS

void reverse_string(char *buff, int len, int str_len) {
    int actual_len = 0;

    // Find the length excluding dots at the end
    for (int i = str_len - 1; i >= 0; i--) {
        if (buff[i] != '.') {
            actual_len = i + 1;
            break;
        }
    }
	char *start = buff;
	char *end = buff + actual_len -1;
	
	// reverse the valid portion of the the buffer in place.
	while (start< end){
	char temp = *start;
	*start = *end;
	*end = temp;

	start++;
	end--;
	}
}

void word_print(char *buff, int len, int str_len) {
    int word_index = 1;
    char *start = NULL;
    int in_word = 0;
    int actual_end = 0;

    // Find where the actual text ends (before the dots)
    for (int i = str_len - 1; i >= 0; i--) {
        if (buff[i] != '.') {
            actual_end = i + 1;
            break;
        }
    }

    printf("Word Print\n----------\n");

    for (int i = 0; i < actual_end; i++) {
        if (buff[i] != ' ') {
            if (!in_word) {
                start = buff + i; // Mark the start of a word
                in_word = 1;
            }
        } else if (in_word) {
            // Mark the end of the word and print it
            printf("%d. ", word_index++);
            for (char *ptr = start; ptr < buff + i; ptr++) {
                putchar(*ptr);
            }
            printf(" (%ld)\n", buff + i - start);

            in_word = 0; // Reset for the next word
        }
    }

    // Handle the last word (if the string doesn't end with a space)
    if (in_word) {
        printf("%d. ", word_index++);
        for (char *ptr = start; ptr < buff + actual_end; ptr++) {
            putchar(*ptr);
        }
        printf(" (%ld)\n", buff + actual_end - start);
    }
}

int search_and_replace(char *buff, int buff_len, char *target, char *replacement){
	int target_len = strlen(target);
	int replacement_len = strlen(replacement);
	char *found = NULL;
	int original_len = 0;
	
	// Calculate the length of the valid part of the buffer
	for(int i = 0; i < buff_len; i++){
		if (buff[i] != '.'){
			original_len++;
		} else{
			break;
		}
	}

	// Find the first occurrence of the target word
	found = strstr(buff, target);
	if (!found){
		printf("Target word not found.\n");
		return 0;

	}

	// Check if the replacement will fit in the buffer
	int new_len = original_len - target_len + replacement_len;
	if (new_len > buff_len){
		printf("Error: Replacement causes bufer overflow.\n");
		return -1;
	}

	// Perform the replacement
	char temp[buff_len];
	int prefix_len = found - buff; // Length of string before the target word

	// Copy part of the buffer before the target word
	strncpy(temp, buff, prefix_len);
	temp[prefix_len] =  '\0';

	//append the replacment word
	strcat(temp, replacement);

	//append the rest of the buffer after the target word
	strcat(temp, found +target_len);

	//copy the modified string back into the buffer
	strncpy(buff, temp, buff_len);


	return new_len;

	}

int main(int argc, char *argv[]){

    char *buff;             //placehoder for the internal buffer
    char *input_string;     //holds the string provided by the user on cmd line
    char opt;               //used to capture user option from cmd line
    int  rc;                //used for return codes
    int  user_str_len;      //length of user supplied string

    //TODO:  #1. WHY IS THIS SAFE, aka what if arv[1] does not exist?
    //      PLACE A COMMENT BLOCK HERE EXPLAINING
	 /*
	 This is safe becasue the condition '(argc > 2)' ensures that 
	 there are at least two arguments: the program name ('argv[0]') and the 
	 option ('argv[1]'). If 'argc < 2', the 'if' block runs, showing usage instructions and exiting the
	 program. This prevents accessing 'argv[1]' when it does not exist.
	 */
    if ((argc < 2) || (*argv[1] != '-')){
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1]+1);   //get the option flag

    //handle the help flag and then exit normally
    if (opt == 'h'){
        usage(argv[0]);
        exit(0);
    }

    //WE NOW WILL HANDLE THE REQUIRED OPERATIONS

    //TODO:  #2 Document the purpose of the if statement below
    //      PLACE A COMMENT BLOCK HERE EXPLAINING

	 /*
		This 'if' statement ensures that the user has providded the minimum required arguments for any operation,
		which is the flag ('argv[1]') and the string ('argv[2]'). If 'argc < 3', it menad that the user has not provided
		a string after the flag , so the program shows usage instructions and exits with an error.
	 */
    if (argc < 3){
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2]; //capture the user input string

    //TODO:  #3 Allocate space for the buffer using malloc and
    //          handle error if malloc fails by exiting with a 
    //          return code of 99
    // CODE GOES HERE FOR #3
	 buff = (char *)malloc(BUFFER_SZ); // allocate memory for the buffer
	 if (buff == NULL){
	 	fprintf(stderr, "Error: Memory allocation failed\n");
		exit(99); // Exit with return code 99
	}

    user_str_len = setup_buff(buff, input_string, BUFFER_SZ);     //see todos
    if (user_str_len < 0){
        printf("Error setting up buffer, error = %d", user_str_len);
        exit(2);
    }

    switch (opt){
        case 'c':
            rc = count_words(buff, BUFFER_SZ, user_str_len);  //you need to implement
            if (rc < 0){
                printf("Error counting words, rc = %d", rc);
                exit(2);
            }
            printf("Word Count: %d\n", rc);
            break;

        //TODO:  #5 Implement the other cases for 'r' and 'w' by extending
        //       the case statement options

		  case 'r':
		  	reverse_string(buff, BUFFER_SZ, user_str_len);
			break;

			case 'w':
				word_print(buff, BUFFER_SZ, user_str_len);
				break;

		   case 'x':
    			if (argc != 5) { // Check if exactly 3 additional arguments are provided
        			fprintf(stderr, "Error: The -x flag requires three arguments: a string, target word, and replacement word.\n");
        			usage(argv[0]); // Print usage instructions
        			exit(1);
    			}

   		 // Placeholder for the actual implementation
	  		//	  printf("Not Implemented!\n");
				char *target = argv[3];
				char *replacement = argv[4];
				int result = search_and_replace(buff, BUFFER_SZ, target, replacement);
				
				// Perform search-and-replace
				if (result < 0){
					fprintf(stderr, "Error: Replacement failed.\n");
					exit(3);
				}

				printf("Modified String: ");
				for(int i = 0; i < result; i++){
					putchar(buff[i]);
				}
				putchar('\n');
	    		break;

        default:
            usage(argv[0]);
            exit(1);
    }

    //TODO:  #6 Dont forget to free your buffer before exiting
    print_buff(buff,BUFFER_SZ);
	 free(buff);
    exit(0);
}

//TODO:  #7  Notice all of the helper functions provided in the 
//          starter take both the buffer as well as the length.  Why
//          do you think providing both the pointer and the length
//          is a good practice, after all we know from main() that 
//          the buff variable will have exactly 50 bytes?
//  
//          PLACE YOUR ANSWER HERE

/*
Providing both the pointer ('buff') and the length ('len') is good practice for the following reasons:
1. the buffer length ('len') allows the functions to avoid overstepping the allocted memory,  ensuring safe ensuring
safe operations and preventing buffer overflows 

2. It helps when when the buffer contains extra padding (e.g '.' characters) since the length tells us the valid range
for processing

3. It makes the functions reuseable for different buffer sizes, improving modularity and robustness.
*/
