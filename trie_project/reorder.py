import random

def shuffle_words(input_file, output_file):
    try:
        # Read words from the input file
        with open(input_file, 'r') as file:
            words = file.read().splitlines()  # Read lines and strip newline characters
        
        # Shuffle the words randomly
        random.shuffle(words)
        
        # Write the shuffled words to the output file
        with open(output_file, 'w') as file:
            file.write('\n'.join(words))
        
        print(f"Words shuffled and written to {output_file}")
    except FileNotFoundError:
        print(f"Error: {input_file} does not exist.")
    except Exception as e:
        print(f"An error occurred: {e}")

# Example usage
input_file = 'prefixes.txt'  # Replace with your input file name
output_file = 'shuffled_words.txt'  # Replace with your output file name

shuffle_words(input_file, output_file)
