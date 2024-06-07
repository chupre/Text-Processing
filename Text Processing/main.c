#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

// Types of options if config file
typedef enum { REPLACE, DELETE, DELETE_FIRSTN, DELETE_LASTN} opt_type;

// Struct for options in config file
typedef struct 
{
	opt_type type;
	char* token;
	char* replace_token;
	int n;
} cfg_opt;

// Displays when keys are not correct or with --help
const char* help =
{
"-с/ --config — ключ для передачи пути к конфигурационному файлу.\n\
-i / --input — ключ для передачи пути к входному файлу.\n\
-o / --output — ключ для передачи пути к выходному файлу.\n\
-h / --help — ключ для вывода справки.\n\
-t / --time — ключ для вывода времени работы.\n\
-s / --show-processing — ключ для вывода процентного соотношения количества обработанного текста\n\n"
};

// Flags set by keys
bool show_time = false, show_processing = false;

FILE* config_file;
FILE* input_file;
FILE* output_file;

void show_help(void);
void cfg_parse_error(void);
void close_files(void);
void process_key(char** keys, int* input_index);
void get_cfg_opt(cfg_opt* opt);
void apply_option(char** line, cfg_opt opt);
void get_input_line(char** dest);
void delete(char* line, char* token);
char* str_replace(char* line, char* token, char* replace_token, int n);
char* str_replace_last(char* line, char* token, char* replace_token, int n);
bool is_sym_num(char sym);

int main(int argc, char** argv)
{
	// Processing keys
	if (argc > 1)
	{
		for (int i = 1; i < argc; i++)
		{	
			process_key(argv, &i);
		}
	}
	else
	{
		show_help();
	}

	// Starts timer if --time is set
	clock_t start, end;

	if (show_time)
	{
		start = clock();
	}

	long input_size = 0;

	if (show_processing)
	{
		fseek(input_file, 0L, SEEK_END);
		input_size = ftell(input_file);
		rewind(input_file);
	}

	// Setting up config options array
	int opt_counter = 0;
	int opts_size = 128;

	cfg_opt* options = malloc(opts_size * sizeof(cfg_opt));

	get_cfg_opt(&options[opt_counter]);

	// Config file parse
	while (options[opt_counter].token[0] != 0)
	{
		opt_counter++;

		if (opt_counter == opts_size)
		{
			opts_size *= 2;
			options = realloc(options, opts_size * sizeof(cfg_opt));
		}

		get_cfg_opt(&options[opt_counter]);
	}

	char* input_line = NULL;
	get_input_line(&input_line);

	// Applying changes based on options line by line
	while (input_line[0] != 0)
	{
		for (int i = 0; i < opt_counter; i++)
		{
			apply_option(&input_line, options[i]);
		}

		if (output_file == NULL)
		{
			printf("%s\n", input_line);
		}
		else
		{
			fprintf(output_file, "%s\n", input_line);
		}

		if (show_processing)
		{
			printf("Progress: %f%%\r", (double)ftell(input_file) / input_size * 100.0);
		}

		if (input_line)
		{
			get_input_line(&input_line);
		}
	}

	free(input_line);

	// Prints time elapsed
	if (show_time)
	{
		end = clock();

		printf("\nTime elapsed: %f\n", (double)(end - start) / CLOCKS_PER_SEC);
	}

	return EXIT_SUCCESS;
}

// Safely close all the files
void close_files(void)
{
	fclose(config_file);
	fclose(input_file);

	if (output_file != NULL)
	{
		fclose(output_file);
	}
}
// Displays help in terminal end exits the program
void show_help(void)
{
	printf("%s", help);
	exit(EXIT_SUCCESS);
}

// Displays error when unable to parse option from config file
void cfg_parse_error(void)
{
	printf("Error parse config");
	exit(EXIT_SUCCESS);
}

// Determines key type and does corresponding actions
void process_key(char** keys, int* input_index)
{
	if (strcmp(keys[*input_index], "-c") == 0 || strcmp(keys[*input_index], "--config") == 0)
	{
		(*input_index)++;
		config_file = fopen(keys[*input_index], "r");
		
		if (config_file == NULL)
		{
			show_help();
		}
	}
	else if (strcmp(keys[*input_index], "-i") == 0 || strcmp(keys[*input_index], "--input") == 0)
	{
		(*input_index)++;
		input_file = fopen(keys[*input_index], "r");

		if (input_file == NULL)
		{
			show_help();
		}
	}
	else if (strcmp(keys[*input_index], "-o") == 0 || strcmp(keys[*input_index], "--output") == 0)
	{
		(*input_index)++;
		output_file = fopen(keys[*input_index], "w");

	}
	else if (strcmp(keys[*input_index], "-h") == 0 || strcmp(keys[*input_index], "--help") == 0)
	{
		show_help();
	}
	else if (strcmp(keys[*input_index], "-t") == 0 || strcmp(keys[*input_index], "--time") == 0)
	{
		show_time = true;
	}
	else if (strcmp(keys[*input_index], "-s") == 0 || strcmp(keys[*input_index], "--show-processing") == 0)
	{
		show_processing = true;
	}
}

// Gets line from input file and stores it in dest
void get_input_line(char** dest)
{
	int size = 512;
	*dest = malloc(size);
	(*dest)[0] = 0;

	char c;
	int counter = 0;

	while ((c = getc(input_file)) != '\n' && c != EOF)
	{
		if (counter == size)
		{
			size *= 2;
			*dest = realloc(*dest, size);
		}

		(*dest)[counter++] = c;
		(*dest)[counter] = 0;
	}
	
}

// Set config option struct members
void get_cfg_opt(cfg_opt* opt)
{	
	int size = 512;
	opt->token = malloc(size);
	opt->token[0] = 0;

	opt->n = -1;

	char c;
	int counter = 0;
	opt_type potential_type = DELETE;
	opt->type = DELETE;

	// Gets line from config file and stores it in option's opt->token
	while ((c = (char)getc(config_file)) != '\n' && c != EOF)
	{
		if (counter == size)
		{
			size *= 2;

			opt->token = realloc(opt->token, size);
		}

		// Start check on DELETE_FIRSTN type
		if (counter == 0 && is_sym_num(c) && potential_type == DELETE)
		{
			char num[256];
			int num_counter = 0;

			while (is_sym_num(c))
			{
				num[num_counter] = c;
				num_counter++;
				c = (char)getc(config_file);
			}

			if (c == '%')
			{
				potential_type = DELETE_FIRSTN;
				opt->n = atoi(num);
				
				if (opt->n == 0)
				{
					cfg_parse_error();
				}

				continue;
			}
			else
			{
				for (int i = 0; i < num_counter; i++)
				{
					opt->token[counter++] = num[i];
					opt->token[counter] = 0;
				}

				if (c != '\n' && c != EOF)
				{
					opt->token[counter++] = c;
					opt->token[counter] = 0;
				}

				continue;
			}
		}

		// End check on DELETE_FIRSTN type
		if (potential_type == DELETE_FIRSTN && c == '%')
		{
			c = (char)getc(config_file);

			if (c == '\n' || c == EOF)
			{
				opt->type = DELETE_FIRSTN;
				break;
			}
			else
			{
				cfg_parse_error();
			}
		}

		// Start check on DELETE_LASTN
		if (counter == 0 && c == '%' && potential_type == DELETE)
		{
			potential_type = DELETE_LASTN;
			continue;
		}

		// End check for DELETE_LASTN here
		if (c == '%' && potential_type == DELETE_LASTN)
		{
			char num[256];
			int num_counter = 0;
			c = (char)getc(config_file);

			while (is_sym_num(c))
			{
				num[num_counter] = c;
				num_counter++;
				c = (char)getc(config_file);
			}

			if (c == '\n' || c == EOF)
			{
				opt->type = DELETE_LASTN;
				opt->n = atoi(num);

				if (opt->n == 0)
				{
					cfg_parse_error();
				}

				break;
			}
			else
			{
				cfg_parse_error();
			}
		}

		// Check on REPLACE
		if (c == '^' && potential_type != REPLACE)
		{
			c = (char)getc(config_file);

			if (c == '\n' || c == EOF)
			{
				cfg_parse_error();
			}
			else
			{
				potential_type = REPLACE;
				opt->type = REPLACE;

				counter = 0;
				size = 512;
				opt->replace_token = malloc(size);

				opt->replace_token[counter++] = c;
				opt->replace_token[counter] = 0;

				while ((c = (char)getc(config_file)) != '\n' && c != EOF)
				{
					if (counter == size)
					{
						size *= 2;

						opt->replace_token = realloc(opt->replace_token, size);
					}

					opt->replace_token[counter++] = c;
					opt->replace_token[counter] = 0;
				}

				break;
			}
		}

		opt->token[counter++] = c;
		opt->token[counter] = 0;
	}
}

// Process option from config file based on it's type and token (or tokens)
void apply_option(char** line, cfg_opt opt)
{
	if (opt.type == DELETE)
	{
		*line = str_replace(*line, opt.token, "", 0);
	}

	if (opt.type == DELETE_FIRSTN)
	{
		*line = str_replace(*line, opt.token, "", opt.n);
	}

	if (opt.type == DELETE_LASTN)
	{
		*line = str_replace_last(*line, opt.token, "", opt.n);
	}

	if (opt.type == REPLACE)
	{
		*line = str_replace(*line, opt.token, opt.replace_token, 0);
	}
}

// Returns true if the character is a digit, else false
bool is_sym_num(char sym)
{
	if (sym > 47 && sym < 58)
	{
		return true;
	}

	return false;
}

// Replaces a substring replace_token in string line.
// n - number - of replacements.
// if n > 0 - n replacements from the start of the line
// if n == 0 - replaces all occurenses of token in line
char* str_replace(char* line, char* token, char* replace_token, int n)
{
	char* result;
	int i, counter = 0;
	size_t token_lenght = strlen(token);
	size_t replace_lenght = strlen(replace_token);

	// Counting the number of times old word 
	// occur in the string 
	if (n == 0)
	{
		for (i = 0; line[i] != '\0'; i++)
		{
			if (strstr(&line[i], token) == &line[i])
			{
				counter++;

				// Jumping to index after the old word. 
				i += token_lenght - 1;
			}
		}
	}
	else
	{
		for (i = 0; line[i] != '\0' && counter < n; i++)
		{
			if (strstr(&line[i], token) == &line[i])
			{
				counter++;

				// Jumping to index after the old word. 
				i += token_lenght - 1;
			}
		}
	}
	

	// Making new string of enough length 
	result = (char*)malloc(strlen(line) + counter * (replace_lenght - token_lenght) + 1);

	i = 0;

	while (*line)
	{
		// compare the substring with the result 
		if (strstr(line, token) == line && counter > 0)
		{
			strcpy(&result[i], replace_token);
			i += replace_lenght;
			line += token_lenght;
			counter--;
		}
		else
		{
			result[i++] = *line++;
		}
	}
	
	result[i] = '\0';
	return result;
}


// Replaces a substring replace_token in string line n times starting from the end.
// n < 0
char* str_replace_last(char* line, char* token, char* replace_token, int n)
{
char* result;
    int i, counter = 0;
    size_t line_length = strlen(line);
    size_t token_length = strlen(token);
    size_t replace_length = strlen(replace_token);

    // Counting the number of times old word 
    // occur in the string 
    for (i = line_length - 1; i >= 0 && counter < n; i--)
    {
        if (strncmp(&line[i], token, token_length) == 0)
        {
            counter++;

            // Jumping to index before the old word. 
            i -= (token_length - 1);

            if (counter == n)
                break; // Stop further search after n matches
        }
    }

    // Making new string of enough length 
    result = (char*)malloc(line_length + 1);

    i = 0;
    while (line_length > 0)
    {
        // compare the substring with the result 
        if (strncmp(&line[line_length - token_length], token, token_length) == 0 && counter > 0)
        {
            strcpy(&result[i], replace_token);
            i += replace_length;
            line_length -= token_length;
            counter--;
        }
        else
        {
            result[i++] = line[--line_length];
        }
    }

    result[i] = '\0';

    // Reverse the result string
    char* reversed_result = (char*)malloc(i + 1);

    for (int j = 0; j < i; j++)
    {
        reversed_result[j] = result[i - j - 1];
    }

    reversed_result[i] = '\0';

    free(result);
    return reversed_result;
}