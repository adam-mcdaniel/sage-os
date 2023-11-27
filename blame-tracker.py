#!/usr/bin/env python3
"""
 _    _                    _               _           
| |__| |__ _ _ __  ___ ___| |_ _ _ __ _ __| |_____ _ _ 
| '_ \ / _` | '  \/ -_)___|  _| '_/ _` / _| / / -_) '_|
|_.__/_\__,_|_|_|_\___|    \__|_| \__,_\__|_\_\___|_|  

Author: Adam McDaniel
File: blame-tracker.py
Date: 01/25/2022
Description: This script accuses the author of commits of all files between two dates in ISO 8601 format.
License: GNU General Public License v3.0
"""

##################################################
# Import libraries
##################################################

import subprocess # For running git commands
import glob       # For finding files
import argparse   # For reading command line arguments
import re         # For extracting the information from the `git blame` output
from sys import stdout, stderr           # For printing to stderr
from os.path import basename, abspath    # For getting the filename from a path
from datetime import datetime, timedelta # For parsing dates

##################################################
# Constants
##################################################

# The name of this program.
PROGRAM_NAME = basename(__file__)
# The default format string for printing accusations.
DEFAULT_FORMAT = '{name:12} ({author} on {date} at {time}): {content}'


##################################################
# Main
##################################################

def main():
    '''
    Main function.

    Parses command line arguments, collects the accusations, and prints them to the output file.
    '''

    ##################################################
    # Parse command line arguments
    ##################################################

    # Create the argument parser
    parser = argparse.ArgumentParser(
        prog = PROGRAM_NAME,
        description='Adam McDaniel <adam-mcdaniel.net> -- This script accuses an author of commits of all files between two dates in ISO 8601 format.')

    # Add arguments to the parser
    parser.add_argument('-r', '--repo', type=str, help='The directory📁 to search for files in (default is ".").', default='./')
    parser.add_argument('-by', '--author', type=str, help='The name📛 of the author to blame', nargs='+', default=[])
    parser.add_argument('-t0', '--since', dest='start_date', type=str, help='The start date📅 in ISO 8601 format.')
    parser.add_argument('-t1', '--until', dest='end_date', type=str, help='The end date📅 in ISO 8601 format (default is now).', default=datetime.now().isoformat())
    parser.add_argument('-d', '--days-ago', type=int, help='The number of days ago⏱️ to shift the search.', default=0)
    parser.add_argument('-w', '--weeks-ago', type=int, help='The number of weeks ago⏱️ to shift the search.', default=0)
    parser.add_argument('-m', '--minutes-ago', type=int, help='The number of minutes ago⏱️ to shift the search.', default=0)
    parser.add_argument('-in', '--include', dest='include', type=str, help='The file patterns📂 to search for (default is "**/*").', default=['**/*'], nargs='+')
    parser.add_argument('-ex', '--exclude', type=str, help='The file patterns📂 to exclude (default is none).', default=[], nargs='+')
    parser.add_argument('-f', '--format', type=str, help=f'The format string🧶 to print each accusation (default is "{DEFAULT_FORMAT}")', default=DEFAULT_FORMAT)
    parser.add_argument('-o', '--output', type=str, help='The output file📝 to write the accusations to.')
    parser.add_argument('-i', '--info', action='store_const', const=1, dest='verbose', help='Print info messagesℹ️ (default is disabled).', default=0)
    parser.add_argument('-v', '--verbose', type=int, choices=range(1, 4), help='Level of verbose output📢 (default is 0).', default=0)
    parser.add_argument('-ws', '--keep-whitespace', action="store_true", help='Keep whitespace in blame output📜 (default is false).', default=False)
    parser.add_argument('-s', '--silence-warnings', action='store_true', help='Silence warnings🔇 (default is false).', default=False)

    # Parse the arguments
    args = parser.parse_args()

    ##################################################
    # Validate arguments
    ##################################################

    # Convert the author names to lowercase
    args.author = [author.lower() for author in args.author]

    if args.verbose >= 3:
        # Print debug information if verbose level is 3 or higher
        info("Calculating start date...")

    # Calculate the start date using the arguments
    if args.start_date or args.days_ago or args.weeks_ago:
        # If a start date is specified, use that.
        # If no start date is specified, shift the current date.
        start_date = parse_date(args.start_date) if args.start_date else datetime.now()
        start_date -= timedelta(days=args.days_ago, weeks=args.weeks_ago)
    else:
        # If no start date is specified, use the beginning of time.
        start_date = datetime.min

    if args.verbose >= 3:
        # Print debug information if verbose level is 3 or higher
        info("Parsing end date...")

    # Parse the end date
    end_date = parse_date(args.end_date)

    # Check that end date is after start date
    if end_date < start_date:
        error('end date must be after start date.')


    ##################################################
    # Begin accusing
    ##################################################

    if args.verbose >= 3:
        # Print debug information if verbose level is 3 or higher
        info("Collecting accusations...")
    # Get the list of files to search
    files = get_files(args.repo, args.include, args.exclude)
    if args.verbose >= 1:
        # Print info messages if verbose level is 1 or higher
        info(f"searching for blame in:")
        for i, file in enumerate(files):
            info(f"   {i+1}. {file}")
    # Collect the accusations
    accusations_by_file = accuse_files(args.repo, files, args.verbose, args.silence_warnings)
    
    if args.verbose >= 3:
        # Print debug information if verbose level is 3 or higher
        info(f"writing accusations for {len(accusations_by_file)} files...")
    
    ##################################################
    # Write the accusations to the output file
    ##################################################

    # Get the output file
    output_file = open(args.output, 'w') if args.output else stdout

    # Print the accusations to the output file
    for file_path, file_lines in accusations_by_file.items():
        # Unpack each accusation
        for author, date, content in file_lines:
            # If the user specified an author, filter out accusations by
            # other authors. If the user did not specify an author, include
            # all accusations.
            if args.author and author.lower() not in args.author:
                if args.verbose >= 3:
                    # Print debug information if verbose level is 3 or higher
                    info(f'ignoring accusation by {author} in {file_path}')
                continue
            
            # If the user specified to ignore whitespace, filter out
            # accusations that are only whitespace.
            if not args.keep_whitespace and content.strip() == '':
                if args.verbose >= 3:
                    # Print debug information if verbose level is 3 or higher
                    info(f'ignoring whitespace accusation by {author} in {file_path}')
                continue

            # If the accusation is within the date range, print it
            if start_date <= date <= end_date:
                # Get the format string specified by the user
                format_string = args.format
                
                # Put the accusation into the desired format
                formatted_line = format_string.format(
                    name=basename(file_path),
                    path=file_path,
                    author=author,
                    content=content,
                    date=f'{date.month:02d}/{date.day:02d}/{date.year}',
                    time=f'{date.hour:02d}:{date.minute:02d}')
                
                # Print the formatted accusation to the output file
                output_file.write(formatted_line + '\n')

    # Close the output file
    if args.output:
        output_file.close()

    ##################################################
    # Print statistics
    ##################################################

    # Collect all the accusations by files into a single list
    all_accusations = [accusation for file_accusations in accusations_by_file.values() for accusation in file_accusations]
    # Filter the accusations by the date range
    accusations_within_timeframe = [a for a in all_accusations if start_date <= a[1] <= end_date]

    if args.verbose:
        # Print the total number of accusations
        authors_stats = analyze_authors(accusations_within_timeframe)

        # Calculate the total number of non-blank characters
        total_non_blank = sum([stats['non-blank'] for stats in authors_stats.values()])

        for author, stats in authors_stats.items():
            # Print the statistics for each author
            info(f'{author}:')
            info(f'    {stats["chars"]} characters')
            info(f'    {stats["lines"]} lines')
            info(f'    {stats["non-blank-lines"]} non-whitespace lines')
            info(f'    {stats["two-or-more-char-lines"]} two-or-more-char lines')
            info(f'    {stats["non-blank"]} non-whitespace characters')
            # Print the percentage of non-blank characters written by the author
            info(f'    Composes {stats["non-blank"] / float(total_non_blank) * 100:2.0f}% of changes since {start_date.month:02d}/{start_date.day:02d}/{start_date.year}')

    if not all_accusations and not args.silence_warnings:
        # Print a message if no authors were found
        warn('No accusations found. Your filters might have excluded all the available files. There also might not be any commits within the specified date range, or at all. Try using the --verbose flag to see more information. Additionally, you might try adjusting the date range using the --start-date and --end-date flags.')
    
    ##################################################
    # End
    ##################################################
    if args.verbose and args.output:
        info(f'Accusations written to {args.output}.')

##################################################
# Helper functions
##################################################

def analyze_author(author: str, accusations: list[tuple[str, datetime, str]]) -> dict[str, int]:
    '''
    Analyzes a list of accusations and returns a dictionary of statistics.

    Parameters
    ----------
    author : str
        The author to filter the accusations by.
    accusations : list[tuple[str, datetime, str]]
        The list of accusations to analyze.
        Each accusation is a tuple of the author, date, and the line content commited.

    Returns
    -------
    dict[str, int]
        A dictionary of statistics for the author.
    '''
    # Convert the author name to lowercase
    author = author.lower()
    # Filter the accusations by the author
    accusations_by_author = [a for a, _, _ in accusations if a.lower() == author]
    # Analyze the accusations
    stats = analyze_authors(accusations_by_author)
    # Return the statistics for the author
    if stats.get(author) is not None:
        return stats[author]
    else:
        # If the author has no accusations, return 0 for all statistics
        return {'lines': 0, 'chars': 0, 'non-blank': 0, 'two-or-more-char-lines': 0, 'non-blank-lines': 0, 'avg-line-len': 0}

def analyze_authors(accusations: list[tuple[str, datetime, str]]) -> dict[str, dict[str, int]]:
    '''
    Analyzes a list of accusations and returns a dictionary of users to dictionaries of statistics.

    Parameters
    ----------
    accusations : list[tuple[str, datetime, str]]
        The list of accusations to analyze.
        Each accusation is a tuple of the author, date, and the line content commited.
    
    Returns
    -------
    dict[str, dict[str, int]]
        A dictionary of statistics.

        The key of the outer dictionary is the author.
        The value of the outer dictionary is a dictionary of statistics for the author.
    '''
    
    # The dictionary of author statistics
    result = {}

    # Calculate the statistics for each author
    for author, _, content in accusations:
        author = author.lower()
        # Get the author's statistics
        result.setdefault(author, {'lines': 0, 'chars': 0, 'two-or-more-char-lines': 0, 'non-blank': 0, 'non-blank-lines': 0, 'avg-line-len': 0})
        result[author]['lines'] += 1
        if content.strip() != '':
            # Check that the line is not blank, then increment the non-blank line count
            result[author]['non-blank-lines'] += 1
        if len(content.strip()) >= 2:
            # Check that the line is at least two characters long, then increment the two-or-more-char line count
            result[author]['two-or-more-char-lines'] += 1
        result[author]['chars'] += len(content)
        result[author]['non-blank'] += len(content.replace(' ', '').replace('\t', ''))

    # Calculate the average line length for each author
    for stats in result.values():
        stats['avg-line-len'] = stats['chars'] / stats['lines']

    # Return the author statistics
    return result

def parse_date(date: str) -> datetime:
    '''
    Parses a date string into a datetime object.

    The date can be formatted either in ISO 8601 format or American date format `m/d/y`.
    
    Parameters
    ----------
    date : str
        The date string to parse.
    '''

    try:
        # Try to parse the date as an ISO 8601 date
        return datetime.fromisoformat(date)
    except ValueError:
        # Try to parse the date as an American date `m/d/y`
        try:
            return datetime.strptime(date, '%m/%d/%Y')
        except ValueError:
            # If the date cannot be parsed, throw an error
            error('date must be in ISO 8601 format or American date format `m/d/y`.')

def get_files(directory = "./", include = ['**/*'], exclude = []) -> list[str]:
    '''
    Gets a list of file paths from a list of file patterns, excluding files matching other file patterns.

    Parameters
    ----------
    directory : str, optional
        The directory to search (default is the current directory)
    include : list[str], optional
        The file patterns to include (default is "**/*")
    exclude : list[str], optional
        The file patterns to exclude (default is none)
    '''

    files = set()

    directory = abspath(directory)

    # Add files matching file patterns
    for file_pattern in include:
        files.update(glob.glob(directory + "/" + file_pattern, recursive=True))
        # Add files in subdirectories
        files.update(glob.glob(directory + "/" + file_pattern + "/**/*", recursive=True))

    # Remove files matching exclude patterns
    for file_pattern in exclude:
        files.difference_update(glob.glob(directory + "/" + file_pattern, recursive=True))
        # Remove files in subdirectories
        files.difference_update(glob.glob(directory + "/" + file_pattern + "/**/*", recursive=True))

    return list(files)

def extract_accusations_from_line_porcelain_output(line_porcelain_output: str) -> list[tuple[str, datetime, str]]:
    '''
    Extracts the author, date, and the line content commited from each line of the `git blame --line-porcelain` output using regex.

    The format of the `git blame --line-porcelain` output can be found here:
    https://git-scm.com/docs/git-blame#_the_porcelain_format

    Parameters
    ----------
    line_porcelain_output : str
        The output of `git blame --line-porcelain`.

    Returns
    -------
    list[tuple[str, datetime, str]]
        A list of tuples containing the author, date, and commited content for each line.
    '''

    # The list of accusations to return
    accusations = []

    # Split the `git blame` output by each starting line of a section.
    #
    # The first line for each section is composed of:
    # 1. 40-byte SHA-1 of the commit the line is attributed to.
    # 2. The line number of the line in the original file.
    # 3. The line number of the line in the final file.
    for i, section in enumerate(re.split('[a-fA-F0-9]{40} \d+ \d+.*', line_porcelain_output)):
        # Skip the first section, which is empty
        if i == 0:
            continue

        # Extract the author, date, filename, and content from the section
        author = re.search('author (.*)', section).group(1)
        date_string = re.search('author-time (\d+)', section).group(1)
        date    = datetime.fromtimestamp(int(date_string))
        content = section.split('\n')[-2].removeprefix('\t')

        # Add the accusation to the list
        accusations.append((author.lower(), date, content))
    
    # Return the list of accusations
    return accusations

def accuse_files(directory: str, files: list[str], verbose: int=0, warnings_disabled=False) -> dict[str, list[tuple[str, datetime, str]]]:
    '''
    Accuses the author of commits of all files between two dates.

    Parameters
    ----------
    directory : str
        The directory to search through for commits.
    files : list[str]
        The files to search through for commits.
    verbose : bool, optional
        Print verbose output (default is false).
    warnings_disabled : bool, optional
        Disable warnings (default is false).
    
    Returns
    -------
    dict[str, list[tuple[str, datetime, str]]]
        A dictionary of accusations.

        The key is the file path, and the value is a list of accusations.
        Each accusation is a tuple of the author, date, and content.
    '''

    directory = abspath(directory)

    # Run git blame on each file
    all_accusations = {}
    for file in files:
        if verbose >= 2:
            # Print file information if verbose level is 2 or higher
            info(f'Checking {file}...')
                
        # Run git blame
        try:
            # First attempt UTF-8 to retain unicode characters
            line_porcelain_output = subprocess.run(['git', 'blame', '--line-porcelain', file], capture_output=True, text=True, cwd=directory, encoding='utf-8').stdout
            if verbose >= 3:
                info("Valid UTF-8 file.")
        except UnicodeDecodeError:
            # If the file is not UTF-8, try latin1 as a fallback
            if not warnings_disabled:
                warn(f'File {file} is not UTF-8. Falling back to latin1 encoding. (You might not want to include this file in your search.)')
            line_porcelain_output = subprocess.run(['git', 'blame', '--line-porcelain', file], capture_output=True, text=True, cwd=directory, encoding='latin1').stdout

        if verbose >= 3:
            # Print debug information if verbose level is 3 or higher
            info("Extracting accusations from line porcelain output...")
        
        # Extract author, date, and content from each line using regex.
        # Then, add the file accusations to the list of all accusations
        all_accusations[file] = extract_accusations_from_line_porcelain_output(line_porcelain_output)

    # Return the list of all accusations
    return all_accusations

def info(*messages: list[str]):
    '''
    Prints an info message to stdout.

    Parameters
    ----------
    messages : list[str]
        The info messages to print.
    '''
    print(f'{PROGRAM_NAME}: info:', *messages, file=stderr, flush=True)

def warn(*messages: list[str]):
    '''
    Prints an info message to stdout.

    Parameters
    ----------
    messages : list[str]
        The info messages to print.
    '''
    print(f'{PROGRAM_NAME}: warning:\x1b[33m', *messages, '\x1b[0m', file=stderr, flush=True)

def error(*messages: list[str]):
    '''
    Prints an error message to stderr.

    Parameters
    ----------
    messages : list[str]
        The error messages to print.
    '''
    print(f'{PROGRAM_NAME}: error:', *messages, 'see `--help` for more information.', file=stderr)
    exit(1)

##################################################
# Main execution
##################################################

if __name__ == '__main__':
    # Run the main function if this file is run as a script
    main()