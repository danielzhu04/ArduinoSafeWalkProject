/*
 * Code for converting RTTTL format to frequency/duration
 * Developed as a helper library for Lab 4 (Clocks, Timers, and Watchdogs)
 * in CSCI 1600 (Real-time and embedded software) at Brown University
 *    (https://cs.brown.edu/courses/info/csci1600/)
 * Author: Milda Zizyte
 */
 
#include "rttl_parser.h"

void printError(int location, char c, String msg) {
  Serial.print("Character (indexed on spaceless string) ");
  Serial.print(location);
  Serial.print(": ");
  Serial.print(msg);
  Serial.print(" [received ");
  Serial.print(c);
  Serial.println("]");
}

/*
 * Convert an RTTTL string to sequential lists of their frequencies (Hz) and durations (ms)
 * A frequency of "0" indicates that the note is actually a rest
 * Inputs:
 *   rtttl: RTTTL music format string
 *      (we allow for notes B0-D#8 inclusive, and for flexibility in dot position)
 *      e.g. if default duration = 4 and octave = 4, a dotted quarter C4 note could be expressed as:
 *      c., c.4, c4., 4.c, 4c., 4.c4, 4c.4, 4c4.
 *   notes: buffer in which to put the frequencies
 *   durs: buffer in which to put the durations
 * Outputs:
 *   length of the song (in notes/rests). A return value of -1 indicates an error.
 */

int rtttlToBuffers(String rtttl, int notes[], int durs[]) {
  int si = 0;

  // convert rtttl string to lowercase and remove spaces
  String rtttlNosp = "";
  for (int i = 0; i < rtttl.length(); i++) {
    if (rtttl[i] >= 'A' and rtttl[i] <= 'Z') {
      rtttlNosp.concat(rtttl[i] - 'A' + 'a');
    } else if (rtttl[i] != ' ') {
      rtttlNosp.concat(rtttl[i]);
    }
  }
  if (rtttlNosp.length() == 0) {
    Serial.println("Cannot parse empty string");
    return -1;
  }
  
  // RTTTL format example
  // spooky:d=4,o=6,b=127:8c,f,8a,f,8c,b5,2g,8f,e,8g,e,8e5,a5,2f,8c,f,8a,f,8c,b5,2g,8f,e,8c,d,8e,1f,8c,8d,8e,8f,1p,8d,8e,8f_
  
  // find starting index (assumes non-empty song name terminated in : character)
  while(rtttlNosp[si] != ':') {
    si += 1;
    if (si >= rtttlNosp.length()) {
      printError(si, '-', "Expected colon after song name");
      return -1;
    }
  }

  // starting index is 1 past index of :
  si += 1;
  // need beat, octave, and duration defined
  if (si >= rtttlNosp.length()) {
    printError(si, '-', "Expected beat, octave, duration");
    return -1;
  }

  // Since b,o,d can appear in any order, we create flags to indicate if they were all found in the string
  bool bFound = false;
  bool oFound = false;
  bool dFound = false;

   // store beat, order, duration when found
  int bod[3];

  while (!bFound or !oFound or !dFound) {
    int bodIndex;
    // check for next character being b, o, or d, and store the corresponding index in bodIndex
    // (b: index 0, o: index 1, d: index 2)
    if (rtttlNosp[si] == 'b') {
      if (bFound) {
        printError(si, 'b', "b defined twice");
        return -1;
      }
      bodIndex = 0;
      bFound = true;
    } else if (rtttlNosp[si] == 'o') {
      if (oFound) {
        printError(si, 'o', "o defined twice");
        return -1;
      }
      bodIndex = 1;
      oFound = true;
    } else if (rtttlNosp[si] == 'd') {
      if (dFound) {
        printError(si, 'd', "d defined twice");
        return -1;
      }
      bodIndex = 2;
      dFound = true;
    } else {
      printError(si, rtttlNosp[si], "Expected b, o, or d");
      return -1;
    }
    // next character expected is =
    si += 1;
    if (si >= rtttlNosp.length() or rtttlNosp[si] != '=') {
      printError(si, rtttlNosp[si], "Expected beat, octave, duration");
      return -1;
    }
    // next character(s) expected are numeric
    si += 1;
    if (si >= rtttlNosp.length() or rtttlNosp[si] < '0' or rtttlNosp[si] > '9') {
      printError(si, rtttlNosp[si], "Expected number for b,o,d");
      return -1;
    }
    // convert the next numeric symbols to a decimal number
    bod[bodIndex] = (int) rtttlNosp[si] - (int) '0';
    si += 1;
    if (si >= rtttlNosp.length()) {
       printError(si - 1, rtttlNosp[si - 1], "Expected beat, order, duration or : after character");
       return -1;
    }
    while (rtttlNosp[si] >= '0' and rtttlNosp[si] <= '9') {
      bod[bodIndex] *= 10;
      bod[bodIndex] += (int) rtttlNosp[si] - (int) '0';
      si += 1;
      if (si >= rtttlNosp.length()) {
        printError(si - 1, rtttlNosp[si - 1], "Expected beat, octave, duration or : after character");
        return -1;
      }
    }

    // next character (after e.g. b=40) is expected to be comma or colon
    if (rtttlNosp[si] != ',' and rtttlNosp[si] != ':') {
      printError(si, rtttlNosp[si], "Expected comma or colon");
      return -1;
    }

    // but we only expect a colon after all of b,o,d are defined
    if (rtttlNosp[si] == ':' and (!bFound or !oFound or !dFound)) {
      printError(si, ';', "Colon before all of b,o,d defined");
      return -1;
    }

    si += 1;
    if (si >= rtttlNosp.length()) {
      printError(si, rtttlNosp[si - 1], "Expected beat, octave, duration or :");
      return -1;
    }
  }
  
  // after we've stored bod and gotten past the :, time to parse the notes
  // From wikipedia:
  // Each note is separated by a comma and includes, in sequence:
  //    a duration specifier (1 = whole note, 2 = half note, etc)
  //    a standard music note, either a, b, c, d, e, f or g*
  //    an octave specifier, as in scientific pitch notation
  // If no duration or octave specifier are present, the default applies.
  //
  // * A # or _ after the note letter indicates a sharp
  // A . after the note indicates dotted notation 
  // We allow for the . to come after the duration, the note letter, or the octave
  
  int noteInd = 0;

  bool prevCommaFound = true;
  while (si < rtttlNosp.length()) {
    if (! prevCommaFound) {
      printError(si, rtttlNosp[si], "Expected commas to separate notes!");
      return -1;
    }
    prevCommaFound = false;
    // If the next symbols are numeric, then we get the duration
    int noteDur = 0;
    bool durFound = false;
    while (rtttlNosp[si] >= '0' and rtttlNosp[si] <= '9') {
      durFound = true;
      noteDur *= 10;
      noteDur += (int) rtttlNosp[si] - (int) '0';
      si += 1;
      if (si >= rtttlNosp.length()) {
        printError(si-1, rtttlNosp[si - 1], "Expected note name");
        return -1;
      } 
    }
    // Compute the duration of the note
    // 1 beat = 1 default duration
    // 1000 * 60 / beat = 1000 * 60 / bod[0] is the duration of one default duration beat in ms
    // defaultDuration * 1000 * 60 / (beat * duration) = bod[2] * 1000 * 60 / (bod[0] * noteDur) is the duration of one beat in ms
    // (for example, if defaultDuration = 4 and nodeDur = 8, then the duration is 1/2 of the default
    if (durFound) { 
      durs[noteInd] = bod[2] * 1000 * 60 / (bod[0] * noteDur);
      // check for dotted notation, which multiplies duration by 1.5
      if (rtttlNosp[si] == '.') {
        durs[noteInd] += durs[noteInd] / 2;
        si += 1;
        if (si >= rtttlNosp.length()) {
          printError(si-1, rtttlNosp[si-1], "Expected note name");
        }
      }
    } else{
      durs[noteInd] = 1000 * 60 / bod[0];
    }

    // Next, parse the note. Pull in the default octave as a starting point
    int noteOctave = bod[1];
    
    // p indicates rest
    if(rtttlNosp[si] == 'p') {
      notes[noteInd] = 0;
      si += 1;
      // rests can be dotted too
      if (si < rtttlNosp.length() and rtttlNosp[si] == '.') {
        durs[noteInd] += durs[noteInd]/2;
        si += 1;
      }
    } else {
      // get the base of the note (a-g)
      if (rtttlNosp[si] < 'a' or rtttlNosp[si] > 'g') {
        printError(si, rtttlNosp[si], "Expected a-g or p");
        return -1;
      }
      char noteBase = rtttlNosp[si];
      si += 1;
      bool sharp = false;
      if (si < rtttlNosp.length()) {
        // detect a sharp
        if (rtttlNosp[si] == '#' or rtttlNosp[si] == '_') {
          sharp = true;
          si += 1;
        }
        // check for dotted notation
        if (si < rtttlNosp.length() and rtttlNosp[si] == '.') {
          durs[noteInd] += durs[noteInd]/2;
          si += 1;
        }
        // check for octave modifier
        // technically RTTTL only allows for octaves 4-7, but we have more notes defined than that so we might
        // as well allow for some flexibility
        if (si < rtttlNosp.length() and rtttlNosp[si] >= '0' and rtttlNosp[si] <= '8') {
          noteOctave = (int) rtttlNosp[si] - (int) '0';
          si += 1;
          // check once more for dotted notation
          if (si < rtttlNosp.length() and rtttlNosp[si] == '.') {
            durs[noteInd] += durs[noteInd]/2;
            si += 1;
          }
        }
      }

      // first note in allNotes is B0 (index of B0 = 0), so
      // "Index" of C0 : - 11
      // Since there are 12 notes in an octave, we compute the index of C[octave] with the following formula:
      int noteDefLocation = -11 + 12 * noteOctave;
      
      // compute offset from c
      // notes go:
      // c, c#, d, d#, e, f, f#, g, g#, a, a#, b
      // 0   1  2   3  4  5   6  7   8  9  10  11
      if (noteBase == 'd') {
        noteDefLocation += 2;
      } else if (noteBase == 'e') {
        noteDefLocation += 4;
      } else if (noteBase == 'f') {
        noteDefLocation += 5;
      } else if (noteBase == 'g') {
        noteDefLocation += 7;
      } else if (noteBase == 'a') {
        noteDefLocation += 9;
      } else if (noteBase == 'b') {
        noteDefLocation += 11;
      }
    
      // this will correctly find the index of a sharp note
      // (even for e# and b#, since e + 1 = f and b + 1 = c in the next octave)
      if (sharp) {
        noteDefLocation += 1;
      }

      if (noteDefLocation < 0 or noteDefLocation >= 89) {
        printError(si, rtttlNosp[si], "Unsupported note frequency");
        return -1;
      }
      
      // look up the note in the allNotes array
      notes[noteInd] = allNotes[noteDefLocation];
    }
 
    // detect comma for error checking at start of loop
    // (this takes care of the fencepost of no comma needed at the end of a song)
    if (si < rtttlNosp.length() and rtttlNosp[si] == ',') {
      prevCommaFound = true;
      si += 1;
    }

    noteInd += 1;
  }

  return noteInd;
}