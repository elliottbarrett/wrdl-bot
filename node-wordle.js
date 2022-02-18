const { WORDS } = require('./dict-wordle')

const POSITION_WEIGHT  = 0.4
const INCLUSION_WEIGHT = 2.7
const PRINT_REMAINING_POSSIBILITIES = false

const LETTERS = 'abcdefghijklmnopqrstuvwxyz'
const UNEVALUATED = 0
const INCORRECT   = 'X'
const WRONG_POS   = '^'
const CORRECT     = '*'

const EMOTES = {
  [INCORRECT]: ':black_large_square:',
  [WRONG_POS]: ':large_yellow_square:',
  [CORRECT]:   ':large_green_square:'
}

// Test algorithm against all words in dictionary
function evaluateAlgorithm(firstWord=null) {
  let totalFails = 0
  let totalGuesses = 0
  let worstGuesses = 0
  let worstWord = ''
  const results = []
  const distribution = [0,0,0,0,0,0]
  const start = Date.now()
  WORDS.forEach(word => {
    const guesses = solveWordle(word, false, firstWord)
    if (guesses > 6) {
      totalFails++
    } else {
      distribution[guesses-1]++
    }
    // results.push({ word, guesses })
    totalGuesses += guesses
    if (worstGuesses < guesses) {
      worstGuesses = guesses
      worstWord = word
    }
  })
  const elapsedMs = Date.now() - start
  console.log(`Evaluated ${WORDS.length} puzzles in ${elapsedMs/1000} seconds (Average: ${elapsedMs/WORDS.length} ms)`)
  console.log(`Worst word '${worstWord}' with ${worstGuesses} guesses, average guesses: ${totalGuesses/WORDS.length}`) 
  console.log(`Guess distribution is: ${distribution} with ${totalFails} failures`)
  console.log(`Algorithm has a ${100*((WORDS.length - totalFails)/WORDS.length)}% success rate\n`)
  return { average: totalGuesses / WORDS.length, failures: totalFails }
}

// Get letter frequencies by position and total
function getLetterFrequencies() {
  const allFreq = {}
  const posFreq = [{},{},{},{},{}]
  WORDS.forEach(word => {
    const tokens = word.split('')
    tokens.forEach((token, index) => {
      allFreq[token] = allFreq[token] ? allFreq[token] + 1 : 1
      posFreq[index][token] = posFreq[index][token] ? posFreq[index][token] + 1 : 1
    })
  })
  return { allFreq, posFreq }
}

// Get list of [word, score] in descending order
function getWordScoreList() {
  const wordScoreList = []
  const { allFreq, posFreq } = getLetterFrequencies()
  const result = {}
  const getWordScore = (word) => {
    let score = 0
    const tokensConsidered = []
    const tokens = word.split('')
    tokens.forEach((token, index) => {
      const tokenFreqAtPos = posFreq[index][token]
      const tokenFreqTotal = allFreq[token]
      score += tokenFreqAtPos * POSITION_WEIGHT
      if (!tokensConsidered.includes(token)) {
        score += tokenFreqTotal * INCLUSION_WEIGHT
      }
      tokensConsidered.push(token)
    })
    return [word, score]
  }
  return WORDS
    .map(getWordScore)
    .sort((a,b) => {
      return a[1] > b[1] ? -1 : 1
    })
}

// Bot algorithm
function solveWordle(target, printResults=true, firstWord=null) {
  if (printResults) {
    console.log(`Target is ${target}`)
  }
  const wordScoreList = getWordScoreList()

  const occurrenceBounds = LETTERS
    .split('')
    .reduce((a,c) => {
      a[c] = { min: 0, max: 5 }
      return a
    }, {})

  const greens  = [null,null,null,null,null] // Found letters
  const guesses = []                         // Guaranteed excludes

  let memoGuessIndex = 0;

  while (true) {
    let guess
    while (true) {
      if (firstWord && guesses.length === 0) {
        guess = firstWord
        break
      }
      const [guessToCheck] = wordScoreList[memoGuessIndex]
      memoGuessIndex++
      if (isViableGuess(guessToCheck, guesses, greens, occurrenceBounds)) {
        guess = guessToCheck
        break
      }
    }

    if (PRINT_REMAINING_POSSIBILITIES) {
      const remainingPossibilities = wordScoreList.filter(([word,score]) => {
        return isViableGuess(word, guesses, greens, occurrenceBounds)
      }).length
      console.log(`\t${remainingPossibilities} possibilities remaining...`)
    }

    const guessTokens = guess.split('')
    const result = evaluateGuess(target, guess)
    guesses.push(guess)
    updateOccurrenceBounds(occurrenceBounds, guess, result)
    result.forEach((hint, index) => {
      if (hint === CORRECT) {
        greens[index] = guessTokens[index]
      }
    })
    if (printResults) {
      printGuessResult(target, guess, guesses.length)
    }

    if (!greens.includes(null)) {
      return guesses.length
    }
  }
}

// Determine if guess is viable
function isViableGuess(guess, priors, greens, occurrenceBounds) {
  const guessTokens = guess.split('')
  if (priors.includes(guess)) {
    return false
  }

  // Check if greens satisfied
  if (greens.find((green, index) => green && guessTokens[index] !== green)) {
    return false
  }

  // Exit if letter constraints not satisfied
  for (const letter in occurrenceBounds) {
    const { min, max } = occurrenceBounds[letter]
    const guessLetterInstances = guessTokens.filter(x => x === letter).length
    if (guessLetterInstances < min || guessLetterInstances > max) {
      return false
    }
  }
  return true
}

// "Occurrence bounds" are the minimum and maximum number of occurences of a letter
//  in the target word, based on the hints received so far. Generally, this range is
//  0..5 inclusive, but receiving a black letter hint will inform the maximum, while
//  receiving green or yellow hints will update the minimum 
function updateOccurrenceBounds(occurrences, guess, result) {
  const guessTokens = guess.split('')
  const guessSet = new Set()
  guessTokens.forEach(l => guessSet.add(l))
  guessSet.forEach(token => {
    const goodCount = guessTokens.filter((x,idx) => x === token && result[idx] !== INCORRECT).length || 0
    const badCount  = guessTokens.filter((x,idx) => x === token && result[idx] === INCORRECT).length || 0
    occurrences[token].min = goodCount
    if (badCount) {
      occurrences[token].max = goodCount
    }
  })
}

// Evaluate a guess
function evaluateGuess(target, guess) {
  const result = Array(5).fill(UNEVALUATED)
  const targetTokens = target.split('')
  const guessTokens = guess.split('')

  const reduceFreq = (acc, cur) => { 
    acc[cur] = acc[cur] ? acc[cur] + 1 : 1
    return acc
  }
  const targetFreq = targetTokens.reduce(reduceFreq, {})
  const guessFreq = targetTokens.reduce(reduceFreq, {})

  const guessesEvaluated = {}

  // Get corrects first
  for (let tokenIndex = 0; tokenIndex < 5; tokenIndex++) {
    const curGuessToken = guessTokens[tokenIndex]
    const curTargetToken = targetTokens[tokenIndex]
    if (curGuessToken === curTargetToken) {
      const guessesSoFar = guessesEvaluated[curGuessToken] || 0
      guessesEvaluated[curGuessToken] = guessesSoFar + 1
      result[tokenIndex] = CORRECT
    }
  }

  // Now evaluate other guesses left to right
  for (let tokenIndex = 0; tokenIndex < 5; tokenIndex++) {
    if (result[tokenIndex] !== UNEVALUATED) {
      continue
    }
    const curGuessToken = guessTokens[tokenIndex]
    const curTargetToken = targetTokens[tokenIndex]
    const guessFreqInTarget = targetFreq[curGuessToken] || 0
    const guessesSoFar = guessesEvaluated[curGuessToken] || 0
    if (guessesSoFar < guessFreqInTarget) {
      result[tokenIndex] = WRONG_POS
    }
    guessesEvaluated[curGuessToken] = guessesSoFar + 1
  }

  for (let tokenIndex = 0; tokenIndex < 5; tokenIndex++) {
    if (result[tokenIndex] === UNEVALUATED) {
      result[tokenIndex] = INCORRECT
    }
  }

  return result
}

// Testing
function printGuessResult(target, guess, guessNum) {
  const result = evaluateGuess(target, guess)
  if (process.argv[3] === '-e') {
    console.log(result.map(r => EMOTES[r]).join(''))
  } else {
    console.log(`guess ${guessNum}: ${guess}, result: ${result.join('')}`)
  }
}

function findBestFirstWord() {
  let bestFirstWord;
  let leastFails = Infinity

  WORDS.forEach(word => {
    console.log(`Evaluating ${word}`)
    const { failures } = evaluateAlgorithm(word)
    if (failures < leastFails) {
      bestFirstWord = word
      leastFails = failures
      console.log(`\n\nFound new best: ${word} -> fails: ${failures}\n\n`)
    }
  })

  console.log(`\n\nBest first word is ${bestFirstWord} with ${leastFails} failures`)
}

if (process.argv[2]) {
  solveWordle(process.argv[2])
} else {
  // evaluateAlgorithm()
  findBestFirstWord()
}