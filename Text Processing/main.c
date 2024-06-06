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
"-с/--config — ключ для передачи пути к конфигурационному файлу.\n\
-i / --input — ключ для передачи пути к входному файлу.\n\
-o / --output — ключ для передачи пути к выходному файлу.\n\
-h / --help — ключ для вывода справки.\n\
-t / --time — ключ для вывода времени работы.\n\
-s / --show-processing — ключ для вывода процентного соотношения количества обработанного текста\n\n"
};

// Flags set by keys
bool show_time = false, show_processing = false;

// Set if no input file
bool terminal_output = false;

FILE* config_file;
FILE* input_file;
FILE* output_file;

void show_help(void);
void process_key(char** keys, int* input_index);
void get_cfg_opt(cfg_opt* opt);
void process_option(cfg_opt opt);
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

	// Config file parse
	cfg_opt option;
	get_cfg_opt(&option);

	while (option.token != NULL)
	{
		process_option(option);
		get_cfg_opt(&option);
	}

	if (show_time)
	{
		end = clock();

		printf("%f", (double)(end - start) / CLOCKS_PER_SEC));
	}

	return EXIT_SUCCESS;
}

// Displays help in terminal end exits the program
void show_help(void)
{
	printf("%s", help);
	exit(EXIT_SUCCESS);
}

// Determines key type and does corresponding actions
void process_key(char** keys, int* input_index)
{
	if (strcmp(keys[*input_index], "-c") == 0 || strcmp(keys[*input_index], "--config"))
	{
		(*input_index)++;
		config_file = fopen(keys[*input_index], "r");
		
		if (config_file == NULL)
		{
			show_help();
		}
	}
	else if (strcmp(keys[*input_index], "-i") == 0 || strcmp(keys[*input_index], "--input"))
	{
		(*input_index)++;
		input_file = fopen(keys[*input_index], "r");

		if (input_file == NULL)
		{
			show_help();
		}
	}
	else if (strcmp(keys[*input_index], "-o") == 0 || strcmp(keys[*input_index], "--output"))
	{
		(*input_index)++;
		output_file = fopen(keys[*input_index], "r");

		if (output_file == NULL)
		{
			terminal_output = true;
		}

	}
	else if (strcmp(keys[*input_index], "-h") == 0 || strcmp(keys[*input_index], "--help"))
	{
		show_help();
	}
	else if (strcmp(keys[*input_index], "-t") == 0 || strcmp(keys[*input_index], "--time"))
	{
		show_time = true;
	}
	else if (strcmp(keys[*input_index], "-s") == 0 || strcmp(keys[*input_index], "--show-processing"))
	{
		show_processing = true;
	}
}

// Set config option struct members
void get_cfg_opt(cfg_opt* opt)
{	
	char* token = opt->token;
	int size = 512;
	token = malloc(size);

	char c;
	int counter;
	opt_type potential_type = DELETE;
	opt->type = DELETE;

	// Gets line from config file and stores it in option's token
	while ((c = getc(config_file)) != '\n' || c != EOF)
	{
		if (counter == size)
		{
			size *= 2;
			token = realloc(token, size);
		}

		// Start check on DELETE_FIRSTN type
		if (counter == 0 && is_sym_num(c))
		{
			char num[256];
			int num_counter = 0;

			while (is_sym_num(c))
			{
				num[num_counter] = c;
				num_counter++;
				c = getc(config_file);
			}

			if (c == '%')
			{
				potential_type = DELETE_FIRSTN;
				opt->n = atoi(num);
			}
			else
			{
				for (int i = 0; i < num_counter; i++)
				{
					token[counter++] = num[i];
				}
			}
		}

		// End check on DELETE_FIRSTN type
		if (potential_type == DELETE_FIRSTN && c == '%')
		{
			c = getc(config_file);

			if (c == '\n' || c == EOF)
			{
				opt->type = DELETE_FIRSTN;
			}
			else
			{
				token[counter++] = '%';
				token[counter++] = c;
				potential_type = DELETE;
			}
		}

		// Start check on DELETE_LASTN
		if (counter == 0 && c == '%')
		{
			potential_type = DELETE_LASTN;
		}

		// End check for DELETE_LASTN here
		if (c == '%' && potential_type == DELETE_LASTN)
		{
			char num[256];
			int num_counter = 0;
			c = getc(config_file);

			while (is_sym_num(c))
			{
				num[num_counter] = c;
				num_counter++;
				c = getc(config_file);
			}

			if (c == '\n' || c == EOF)
			{
				opt->type = DELETE_LASTN;
				opt->n = atoi(num);
			}
			else
			{
				potential_type = DELETE;

				token[counter++] = '%';

				for (int i = 0; i < num_counter; i++)
				{
					token[counter++] = num[i];
				}
			}
		}

		// Check on REPLACE (highest priority)
		if (c == '^' && potential_type != REPLACE)
		{
			c = getc(config_file);

			if (c == '\n' || c == EOF)
			{
				break;
			}
			else
			{
				potential_type = REPLACE;
				opt->type = REPLACE;

				token = opt->replace_token;
				size = 512;
				token = malloc(size);

				counter = 0;
			}
		}

		token[counter++] = c;
	}
}

// Process option from config file based on it's type and token (or tokens)
void process_option(cfg_opt opt)
{

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