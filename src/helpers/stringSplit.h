/*
 * Copyright 2024 NearEDGE, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <deque>
#include <algorithm>

namespace misc {
  template <typename T, typename C = std::deque<T>>
  C split(T line, const char delim, bool stripLeading = false, const char escape = '\0');

  // We create our own version if std::adjacent_find that supports the notion of an escape sequence and
  // hence is able to skip ahead by more than 1 position
  template <typename _InputIterator, typename _Predicate>
  inline _InputIterator adjacent_find_escape(_InputIterator first, _InputIterator last, _Predicate pred) {

    while (first < last) {
      std::size_t       count = (std::size_t)(last - first);
      // Are we last
      if ( count == 0 ) {
        first = last;   // Not found
        break;
      }

      // Evaluate the predicate
      std::size_t     increment = pred(*first, *(first+1), count);
      if (increment == 0) {
        break;
      }

      // Most of the time the increment should be 1
      if (increment == 1) {
        ++first;
        continue;
      }

      // Increment by more than 1. We must make sure not to go beyond last
      if ( increment <= count ) {
        first += increment;
        continue;
      } else {
        first = last;
        break;
      }
    }

    return first;
  }

  template <typename T, typename C>
  inline C split(T line, const char delim, bool stripLeading, const char escape) {
    C                         result;
    typename T::iterator      start = line.begin();
    // Remove the leading delim.
    if (stripLeading == true) {
      std::size_t   removeCount = 0;
      while (start != line.end()) {
        if ((*start) == delim) {
          ++removeCount;
          ++start;
        } else {
          break;
        }
      }
      if (removeCount != 0)
        line.erase(0, removeCount);
    }

    // Actual split on the trimmed line
    start = line.begin();
    while (start != line.end()) {
      typename T::iterator it;
      if (escape == '\0') {
        it = std::find_if(start, line.end(), [delim, escape](const char &v) -> bool { return (v == delim); } );
      } else {
        it = adjacent_find_escape(start, line.end(), [delim, escape](const char& f, const char& s, std::size_t c) -> std::size_t { if ( (c >= 2) and (f == escape) and (s == delim) ) return 2; if (f == delim) return 0; return 1; } );
      }
      result.push_back(T(start, it));

      // We are done, no more data
      if (it == line.end())
        break;

      // We coalesce the delimiter by moving ahead until we find something different than the delimiter.
      //while ( (*it == delim) and (it != line.end())) {
      //  ++it;
      //}
      //start = it;
      //if (escape == '\0') {
        start = std::find_if(it, line.end(), [delim, escape](const char &v) -> bool { return (v != delim); } );
      //} else {
      //  start = adjacent_find_escape(it, line.end(), [delim, escape](const char& f, const char& s) -> std::size_t { if ( (f == escape) and (s == delim) ) return 2; if (s != delim); return true; } );
      //}
    }

    return result;
  }

} // namespace misc


