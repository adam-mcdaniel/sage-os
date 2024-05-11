/**
 * @file path.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Path splitting utilities.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

/**
 * @brief Skip all preceding slashes
 * @param path the input path to skip
 * @return the pointer moved passed the slashes.
 */
const char *path_skip_slashes(const char *path);

/**
 * @brief Skip all characters until the next slash.
 * @param path the input path to skip until the next slash.
 * @return the pointer moved to the next slash.
 */
const char *path_next_slash(const char *path);

/**
 * @brief Retrieve only the file at the end of a path.
 * @param path the full path.
 * @return the pointer moved to just the filename portion of the path.
 */
const char *path_file_name(const char *path);

/**
 * @brief Retrieve only the file at the end of a path.
 * @param path the full path.
 * @return the pointer moved to just the filename portion of the path.
 */
struct List;
struct List *path_split(const char *path);
void path_split_free(struct List *l);
