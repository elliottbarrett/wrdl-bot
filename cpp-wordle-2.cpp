#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <chrono>

// Types and enums
enum Hint {
  UNEVALUATED = 0,
  INCORRECT,
  WRONG_POS,
  CORRECT
};

struct GuessResult {
  Hint hints[5];

  void print() {
    for (int i=0; i < 5; i++) {
      switch (hints[i]) {
        case INCORRECT:
          std::cout << 'X';
          break;
        case WRONG_POS:
          std::cout << '^';
          break;
        case CORRECT:
          std::cout << '*';
          break;
        default:
          std::cout << '?';
          break;
      }
    }
  }

  bool isAllCorrect() {
    for (int i=0; i < 5; i++) {
      if (hints[i] != CORRECT) {
        return false;
      }
    }
    return true;
  }
};

struct LetterFrequencies {
  std::map<char, int> all;
  std::map<char, int> pos[5];
};

struct WordScore {
  std::string word;
  float score;
};

struct Range {
  int min;
  int max;
};

// Constants
const std::string LETTERS = "abcdefghijklmnopqrstuvwxyz";
double POSITION_WEIGHT = 0.4;
double INCLUSION_WEIGHT = 2.7;

// Globals
std::vector<std::string> WORDS;
std::vector<WordScore> WORD_SCORE_LIST;
LetterFrequencies LETTER_FREQUENCIES;
std::map<Hint, char> HINT_CHARS;

// Evaluation functions
std::vector<std::string> readWordFile() {
  std::fstream file;
  std::string filename = "dict-wordle.txt";
  std::vector<std::string> fileLines;
  std::string line;

  file.open(filename, std::ios::in);
  if (file) {
    while (getline(file, line)) {
      fileLines.push_back(line);
    }
  }

  return fileLines;
}

std::map<char, Range> buildInitialOccurrenceBounds() {
  std::map<char, Range> bounds;
  for (int i=0; i < 26; i++) {
    char c = LETTERS.at(i);
    Range r;
    r.min = 0;
    r.max = 5;
    bounds[c] = r;
  }
  return bounds;
}

LetterFrequencies buildLetterFrequencies() {
  LetterFrequencies freq;
  for (auto & word : WORDS) {
    for (int i=0; i < 5; i++) {
      char token = word.at(i);
      freq.all[token]++;
      freq.pos[i][token]++;
    }
  }
  return freq;
}

bool compareWordScore(WordScore a, WordScore b) {
  return a.score > b.score;
}

std::vector<WordScore> buildWordScoreList() {
  std::vector<WordScore> result;
  for (auto & word : WORDS) {
    int score = 0;
    std::map<char, int> tokensConsidered;
    for (int i=0; i < 5; i++) {
      char token = word.at(i);
      int tokenFreqAtPos = LETTER_FREQUENCIES.pos[i][token];
      int tokenFreqTotal = LETTER_FREQUENCIES.all[token];
      score += (float)tokenFreqAtPos * POSITION_WEIGHT;
      if (tokensConsidered[token] == 0) {
        score += (float)tokenFreqTotal * INCLUSION_WEIGHT;
      }
      tokensConsidered[token]++;
    }
    WordScore wordScore;
    wordScore.word = word;
    wordScore.score = score;
    result.push_back(wordScore);
  }

  std::sort(result.begin(), result.end(), compareWordScore);
  return result;
}

bool isViableGuess(std::string &guess, char greens[], std::map<char, Range> &occurrenceBounds) {
  // Check if greens satisfied
  for (int i=0; i < 5; i++) {
    char green = greens[i];
    if (green != 0 && guess.at(i) != green) {
      return false;
    }
  }

  // Exit if letter constraints not satisfied
  for (int i=0; i < 26; i++) {
    char letter = 'a' + i;
    Range bounds = occurrenceBounds[letter];
    int guessLetterInstances = 0;
    for (int j=0; j < 5; j++) {
      if (guess.at(j) == letter) {
        guessLetterInstances++;
      }
    }
    if (guessLetterInstances < bounds.min || guessLetterInstances > bounds.max) {
      return false;
    }
  }
  return true;
}

void updateOccurrenceBounds(std::map<char, Range> &occurrenceBounds, std::string &guess, GuessResult &result) {
  std::map<char, int> guessTokenSet;
  for (int i=0; i < 5; i++) {
    char guessToken = guess.at(i);
    int goodCount = 0;
    int badCount = 0;
    for (int j=0; j < 5; j++) {
      if (guess.at(j) == guessToken && result.hints[j] != INCORRECT) {
        goodCount++;
      }
      if (guess.at(j) == guessToken && result.hints[j] == INCORRECT) {
        badCount++;
      }
      occurrenceBounds[guessToken].min = goodCount;
      if (badCount > 0) {
        occurrenceBounds[guessToken].max = goodCount;
      }
    }
  }
}

GuessResult evaluateGuess(std::string &target, std::string &guess) {
  GuessResult result;
  std::map<char, int> targetFreq;
  std::map<char, int> guessesEvaluated;

  // Build target token frequency map
  for (int i=0; i < 5; i++) {
    char targetToken = target.at(i);
    targetFreq[targetToken]++;
    result.hints[i] = UNEVALUATED;
  }

  // Get corrects first
  for (int i=0; i < 5; i++) {
    char curGuessToken = guess.at(i);
    char curTargetToken = target.at(i);
    if (curGuessToken == curTargetToken) {
      guessesEvaluated[curGuessToken]++;
      result.hints[i] = CORRECT;
    }
  }

  // Now get letters with wrong positioning
  for (int i=0; i < 5; i++) {
    if (result.hints[i] != UNEVALUATED) {
      continue;
    }
    char curGuessToken = guess.at(i);
    int guessFreqInTarget = targetFreq[curGuessToken];
    int guessesSoFar = guessesEvaluated[curGuessToken];
    if (guessesSoFar < guessFreqInTarget) {
      result.hints[i] = WRONG_POS;
    }
    guessesEvaluated[curGuessToken]++;
  }

  // Everything else is just incorrect
  for (int i=0; i < 5; i++) {
    if (result.hints[i] == UNEVALUATED) {
      result.hints[i] = INCORRECT;
    }
  }

  return result;
}

int solveWordle(std::string target, bool printGuesses) {
  std::map<char, Range> occurrenceBounds = buildInitialOccurrenceBounds();
  char greens[5];
  std::vector<std::string> guesses;

  for (int i=0; i < 5; i++) {
    greens[i] = 0;
  }

  int memoGuessIndex = 0;
  while (true) {
    // Find viable guess
    std::string guess;
    while (true) {
      if (guesses.size() == 0) {
        guess = "perch";
        break;
      } else if (guesses.size() == 1) {
        guess = "mangy";
        break;
      } else if (guesses.size() == 2) {
        guess = "doubt";
        break;
      }
      WordScore wordScore = WORD_SCORE_LIST[memoGuessIndex];
      memoGuessIndex++;
      if (isViableGuess(wordScore.word, greens, occurrenceBounds)) {
        guess = wordScore.word;
        break;
      }
    }

    GuessResult result = evaluateGuess(target, guess);
    guesses.push_back(guess);
    updateOccurrenceBounds(occurrenceBounds, guess, result);

    if (printGuesses) {
      std::cout << "guess " << guesses.size() << ": " << guess << ", result: ";
      result.print();
      std::cout << std::endl;
    }

    // Update greens
    for (int i=0; i < 5; i++) {
      if (result.hints[i] == CORRECT) {
        greens[i] = guess.at(i);
      }
    }

    if (result.isAllCorrect()) {
      return guesses.size();
    }
  }
  return 0;
}

int evaluateAlgorithm() {
  int totalFails = 0;
  int totalGuesses = 0;
  int worstGuesses = 0;
  int dist[6];
  for (int i=0; i < 6; i++) {
    dist[i] = 0;
  }
  std::string worstWord;
  auto start = std::chrono::steady_clock::now();
  for (auto & word : WORDS) {
    // std::cout << "Solving " << word << std::endl;
    int guesses = solveWordle(word, false);
    if (guesses > 6) {
      totalFails++;
    } else {
      dist[guesses-1]++;
    }
    totalGuesses += guesses;
    if (worstGuesses < guesses) {
      worstGuesses = guesses;
      worstWord = word;
    }
  }
  auto end = std::chrono::steady_clock::now();
  auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  // std::cout << "evaluating " << fourthWord << std::endl;
  std::cout << "Evaluated " << WORDS.size() << " puzzles in " << (float)elapsedMs/1000 << " seconds (Average: " << (float)elapsedMs / WORDS.size() << " ms)" << std::endl;
  std::cout << "Worst word " << worstWord << " with " << worstGuesses << " guesses, average guesses " << (double)totalGuesses/WORDS.size() << std::endl;
  std::cout << "Guess distribution is: " << dist[0] << "," << dist[1] << "," << dist[2] << "," << dist[3] << "," << dist[4] << "," << dist[5] << " with " << totalFails << " failures" << std::endl;
  std::cout << "Algorithm has a " << 100*(double)(WORDS.size() - totalFails)/WORDS.size() << "\% success rate" << std::endl;

  return totalFails;
}

// void findBestFourthWord() {
//   std::string bestFirst;
//   int leastFails = INT_MAX;
//   for (auto & word : WORDS) {
//     int fails = evaluateAlgorithm(word);
//     if (fails < leastFails) {
//       leastFails = fails;
//       bestFirst = word;
//       std::cout << "\n  New best word: " << word << " with " << leastFails << " failures" << std::endl << std::endl;
//     }
//   }
//   std::cout << "Found best word... " << bestFirst << " with " << leastFails << " failures" << std::endl;
// }

// Entry point
int main(int argc, char **argv) {
  WORDS = readWordFile();
  LETTER_FREQUENCIES = buildLetterFrequencies();
  WORD_SCORE_LIST = buildWordScoreList();
  if (argc > 1) {
    std::string asdf = "";
    solveWordle(std::string(argv[1]), true);
  } else {
    // findBestFourthWord();
    evaluateAlgorithm();

  }
  return 0;
}