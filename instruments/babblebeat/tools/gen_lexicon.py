#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""BabbleBeat's lexicon: lang/babble_words.h, from the hand-made core below and two public word lists.

    pip install cmudict nltk            (and NLTK's 'wordnet' corpus where nltk.data can find it)
    python3 instruments/babblebeat/tools/gen_lexicon.py

What comes from where:
  - how a word is said: the CMU Pronouncing Dictionary, its sounds respelled in the voice's own letters (SOUND below);
  - what kind of word it is, whether a verb takes an object, and which world a noun belongs to: Princeton WordNet 3.0
    (a noun's world is the lexicographer file of its commonest sense; a world's words are in the order of WordNet's
    tree, so that the words beside each other are kin);
  - which words are common enough to keep: WordNet's sense counts.
The first four worlds, twelve nouns each, and the first adjectives and verbs are BabbleBeat's own, as they were.
The output is committed: the firmware build needs neither list.  See lang/LEXICON-SOURCES.md for the notices."""
import re, sys
from collections import defaultdict
from pathlib import Path
import cmudict
from nltk.corpus import wordnet as wn

HERE = Path(__file__).resolve().parent
OUT = HERE.parent / "lang" / "babble_words.h"

# ---- the hand-made core (BabbleBeat 0.1 to 0.4): text, the voice's spelling ----
CORE_WORLDS = [
    ("MACHINE", [("machine", "mUSin"), ("signal", "sIgnUl"), ("circuit", "sErkIt"), ("engine", "enJIn"), ("robot", "r5bat"), ("speaker", "spikEr"), ("voltage", "v5ltIJ"), ("static", "stAtIk"), ("terminal", "tErmInUl"), ("mirror", "mIrEr"), ("antenna", "AntenU"), ("filter", "fIltEr")]),
    ("DREAM", [("ghost", "g5st"), ("shadow", "SAd5"), ("moon", "mun"), ("window", "wInd5"), ("planet", "plAnIt"), ("garden", "gardUn"), ("ritual", "rICuUl"), ("memory", "memEri"), ("door", "dor"), ("fog", "fag"), ("language", "lANgwIJ"), ("dream", "drim")]),
    ("BODY", [("mouth", "m3T"), ("throat", "Tr5t"), ("tongue", "tUN"), ("bone", "b5n"), ("skin", "skIn"), ("nerve", "nErv"), ("lung", "lUN"), ("insect", "Insekt"), ("vowel", "v3Ul"), ("breath", "breT"), ("spine", "sp2n"), ("voice", "v4s")]),
    ("GOTHIC", [("castle", "kAsUl"), ("coffin", "kOfIn"), ("vampire", "vAmp2r"), ("crypt", "krIpt"), ("blood", "blUd"), ("wolf", "wVlf"), ("night", "n2t"), ("diary", "d2Uri"), ("chapel", "CApUl"), ("letter", "letEr"), ("count", "k3nt"), ("grave", "gr1v")]),
]
CORE_ADJ = [("pale", "p1l"), ("electric", "IlektrIk"), ("ancient", "1nSUnt"), ("silent", "s2lUnt"), ("hidden", "hIdUn"), ("crooked", "krVkId"), ("synthetic", "sInTetIk"), ("hollow", "hal5"), ("broken", "br5kUn"), ("restless", "restlIs"), ("silver", "sIlvEr"), ("strange", "str1nJ")]
CORE_INTR = [("waits", "wait", "w1ts", "w1t"), ("sleeps", "sleep", "slips", "slip"), ("whispers", "whisper", "wIspErz", "wIspEr"), ("dreams", "dream", "drimz", "drim"), ("shivers", "shiver", "SIvErz", "SIvEr"), ("sings", "sing", "sINz", "sIN"), ("returns", "return", "rItErnz", "rItErn"), ("vanishes", "vanish", "vAnISIz", "vAnIS")]
CORE_TRANS = [("remembers", "remember", "rImembErz", "rImembEr"), ("follows", "follow", "fal5z", "fal5"), ("haunts", "haunt", "hOnts", "hOnt"), ("watches", "watch", "waCIz", "waC"), ("forgets", "forget", "fErgets", "fErget"), ("opens", "open", "5pUnz", "5pUn"), ("swallows", "swallow", "swal5z", "swal5"), ("corrupts", "corrupt", "kUrUpts", "kUrUpt")]

# ---- the worlds made from WordNet: name, the lexicographer files whose nouns it takes ----
WORLDS = [
    ("ANIMAL", ["noun.animal"]), ("PLANT", ["noun.plant"]), ("FOOD", ["noun.food"]), ("THING", ["noun.artifact"]),
    ("PLACE", ["noun.location", "noun.object"]), ("MATTER", ["noun.substance", "noun.phenomenon"]),
    ("TIME", ["noun.time"]), ("MIND", ["noun.cognition", "noun.feeling"]), ("TALK", ["noun.communication"]),
    ("DEED", ["noun.act", "noun.event"]), ("PERSON", ["noun.person"]), ("GROUP", ["noun.group"]),
    ("FLESH", ["noun.body"]), ("SHAPE", ["noun.shape", "noun.relation"]), ("TRAIT", ["noun.attribute"]),
    ("STATE", ["noun.state", "noun.process"]), ("MONEY", ["noun.possession", "noun.quantity"]),
]
PER_WORLD, N_ADJ, N_TRANS, N_INTR, N_ADV = 450, 1400, 800, 450, 350
# which worlds a kind of verb suits (WordNet's verb files): "the toad eats the bean" sooner than "the toad audits it"
SUITS = {
    "verb.consumption": "FOOD ANIMAL FLESH BODY", "verb.communication": "TALK PERSON GROUP",
    "verb.cognition": "MIND PERSON DREAM", "verb.emotion": "MIND PERSON DREAM GOTHIC",
    "verb.perception": "ANIMAL PERSON DREAM GOTHIC MACHINE", "verb.motion": "ANIMAL PERSON PLACE MACHINE THING",
    "verb.weather": "MATTER PLACE TIME", "verb.creation": "THING PERSON MACHINE TALK",
    "verb.body": "FLESH BODY ANIMAL PERSON", "verb.possession": "MONEY PERSON GROUP",
    "verb.social": "PERSON GROUP DEED", "verb.change": "MATTER TIME STATE PLANT SHAPE TRAIT",
    "verb.contact": "THING MACHINE FLESH BODY", "verb.competition": "DEED GROUP PERSON ANIMAL",
}
ADVERBS = set("""always never often again soon twice once forever somewhere nowhere everywhere together alone away
backward forward upward downward sideways tonight tomorrow yesterday sometimes anyway elsewhere underground overhead
indoors outdoors abroad aloud apart ahead afterward""".split())
PREPOSITIONS = """inside under over behind beside near without against beyond through across toward above below among
around before after with upon within outside past beneath between into onto underneath despite like""".split()

# ---- the voice's letters for the dictionary's sounds (dsp/babble_voice.c PH[]) ----
SOUND = {"AA": "a", "AE": "A", "AH": "U", "AO": "O", "AW": "3", "AY": "2", "EH": "e", "ER": "Er", "EY": "1", "IH": "I",
         "IY": "i", "OW": "5", "OY": "4", "UH": "V", "UW": "u", "B": "b", "CH": "C", "D": "d", "DH": "D", "F": "f",
         "G": "g", "HH": "h", "JH": "J", "K": "k", "L": "l", "M": "m", "N": "n", "NG": "N", "P": "p", "R": "r",
         "S": "s", "SH": "S", "T": "t", "TH": "T", "V": "v", "W": "w", "Y": "y", "Z": "z",
         "ZH": "z"}                                 # (the voice has no "zh": "measure" with the z of "maze")
VOICE = set("iIeAaOUVuEo12345wyrlmnNsSfTzvDhptkbdgCJ")

# ---- words a talking toy is better without: the rude, the cruel, the clinical, and words about kinds of people ----
BLOCK = set("""abortion abuse addict alcohol anal anus arse ass assault bastard bitch bleed bloody bomb boob breast
bugger bullet burial butt buttock cancer casualty cocaine cock corpse crap cripple cum damn dead death devil dick
die disease drug drunk dung dyke enema erect erection execute fag fart fat feces fetus foreskin fuck gay genital
genitals god groin gun hell heroin hitler homo hooker horny incest jew kill killer killing knife lust massacre
molest murder naked nazi negro nigger nipple nude orgasm pee penis piss poison poop porn pregnant prick prostitute
pubic puke pussy queer rape rapist rectum retard scrotum semen sex sexual sexy shit shoot slave slaughter slut
sperm stab strangle suicide testicle tit torture urine uterus vagina victim virgin vomit vulva war weapon whore womb
wound nazi terrorist terror hostage disabled blind deaf dumb lame insane crazy mad idiot stupid moron ugly
christian muslim jewish catholic black white brown yellow race racial ethnic native foreign alien savage
abort hang hanging beat beating whip rifle pistol cannon bayonet grenade missile gallows noose""".split())
STOP_ADJ = set("""other such same own many few several more most much last next first second third only able due
certain various whole main former following previous present particular general early late recent major minor
total single double further available possible likely necessary responsible similar different special specific
respective least less latter prior half both each every either neither enough little great good bad new old big
small high low long short right left real sure""".split())
NUMBER = re.compile(r"^(one|two|three|four|five|six|seven|eight|nine|ten|eleven|twelve|\w+teen|\w+ty|hundred|thousand|"
                    r"million|billion|\w+(ie)?th|zero|dozen)$")
STOP_VERB = set("be have do can will may must shall get let make go come take give say see know think use need want".split())

try:
    from better_profanity import profanity
    profanity.load_censor_words()
    rude = lambda w: w in BLOCK or profanity.contains_profanity(w)
except Exception:                                   # (the list above alone)
    rude = lambda w: w in BLOCK

CMU = cmudict.dict()


def say(word):
    """the voice's spelling of a word, or None (not in the dictionary, a sound the voice lacks, or too long)"""
    prons = CMU.get(word)
    if not prons:
        return None
    p = [re.sub(r"\d", "", s) for s in prons[0]]
    if any(s not in SOUND for s in p) or len(p) > 10:
        return None
    out = ""
    for i, s in enumerate(p):
        out += "o" if s == "AO" and i + 1 < len(p) and p[i + 1] == "R" else SOUND[s]
    assert set(out) <= VOICE, (word, out)
    return out


def plain(word, lo=3, hi=9):
    return (word.isalpha() and word.islower() and word.isascii() and lo <= len(word) <= hi
            and re.search(r"[aeiouy]", word) and not rude(word))


def count(word, pos):
    return sum(l.count() for s in wn.synsets(word, pos) for l in s.lemmas() if l.name() == word)


def third(base):
    if re.search(r"(s|x|z|ch|sh|o)$", base):
        return base + "es"
    if re.search(r"[^aeiou]y$", base):
        return base[:-1] + "ies"
    return base + "s"


def nouns_of(files, taken):
    best = {}
    for s in wn.all_synsets("n"):
        if s.lexname() not in files or s.instance_hypernyms():
            continue
        for l in s.lemmas():
            w = l.name()
            if w in taken or not plain(w) or wn.synsets(w, "n")[0] != s:      # (its commonest sense is this one)
                continue
            n, v, a = count(w, "n"), count(w, "v"), count(w, "a") + count(w, "s")
            if n < v or n < a or not say(w) or re.search(r"[^su]s$", w):   # (no plurals: "cows")
                continue
            path = max(s.hypernym_paths(), key=len)
            best[w] = (n * 8 + len(wn.synsets(w)), "/".join(x.name() for x in path))   # (how common: counted uses, then senses)
    keep = sorted(best, key=lambda w: (-best[w][0], len(w), w))[:PER_WORLD]
    return sorted(keep, key=lambda w: (best[w][1], w))                         # (the tree's order: kin side by side)


def adjectives(taken):
    best = {}
    for s in list(wn.all_synsets("a")) + list(wn.all_synsets("s")):
        for l in s.lemmas():
            w = l.name()
            if w in taken or w in STOP_ADJ or NUMBER.match(w) or not plain(w):
                continue
            a = count(w, "a") + count(w, "s")
            if a < count(w, "n") or a < count(w, "v") or not say(w):
                continue
            best[w] = a * 8 + len(wn.synsets(w))
    return sorted(sorted(best, key=lambda w: (-best[w], len(w), w))[:N_ADJ])


def adverbs(taken):
    best = {}
    for s in wn.all_synsets("r"):
        for l in s.lemmas():
            w = l.name()
            if w in taken or not plain(w, 3, 10) or not (w.endswith("ly") or w in ADVERBS) or not say(w):
                continue
            best[w] = count(w, "r") * 8 + len(wn.synsets(w))
    return sorted(sorted(best, key=lambda w: (-best[w], len(w), w))[:N_ADV])


def verbs(taken, bit):
    trans, intr, suits = {}, {}, {}
    for s in wn.all_synsets("v"):
        for l in s.lemmas():
            w = l.name()
            if w in taken or w in STOP_VERB or not plain(w, 3, 8) or wn.synsets(w, "v")[0] != s:
                continue
            v = count(w, "v")
            t = third(w)
            if v < 1 or v < count(w, "n") or rude(t) or not say(w) or not say(t):
                continue
            suits[w] = sum(bit[x] for x in SUITS.get(s.lexname(), "").split())
            frames = set(l.frame_ids()) | set(s.frame_ids())
            if frames & {8, 9, 10, 11}:                 # "Somebody ----s something" and its like
                trans[w] = v
            elif frames & {1, 2}:                       # "Something ----s", "Somebody ----s"
                intr[w] = v
    pick = lambda d, n: sorted(sorted(d, key=lambda w: (-d[w], len(w), w))[:n])
    return ([(third(w), w, say(third(w)), say(w), suits[w]) for w in pick(trans, N_TRANS)],
            [(third(w), w, say(third(w)), say(w), suits[w]) for w in pick(intr, N_INTR)])


def main():
    taken = {w for _, ws in CORE_WORLDS for w, _ in ws} | {w for w, _ in CORE_ADJ} | {v[1] for v in CORE_INTR + CORE_TRANS}
    for ws in ([w for _, x in CORE_WORLDS for w in x] + CORE_ADJ):
        assert set(ws[1]) <= VOICE, ws
    worlds = [(name, list(ws)) for name, ws in CORE_WORLDS]
    for name, files in WORLDS:
        ws = nouns_of(files, taken)
        taken |= set(ws)
        worlds.append((name, [(w, say(w)) for w in ws]))
    adj = CORE_ADJ + [(w, say(w)) for w in adjectives(taken)]
    taken |= {w for w, _ in adj}
    adv = [(w, say(w)) for w in adverbs(taken)]
    prep = [(w, say(w)) for w in PREPOSITIONS]
    assert all(p for _, p in prep)
    bit = {name: 1 << k for k, (name, _) in enumerate(worlds)}
    t, i = verbs(taken | {w for w, _ in adv} | set(PREPOSITIONS), bit)
    trans, intr = [v + (0,) for v in CORE_TRANS] + t, [v + (0,) for v in CORE_INTR] + i
    q = lambda s: '"' + s + '"'
    o = ["/* SPDX-License-Identifier: MPL-2.0",
         " * BabbleBeat's words. GENERATED by tools/gen_lexicon.py: change that, not this.",
         " * Pronunciations from the CMU Pronouncing Dictionary; word classes and worlds from Princeton WordNet 3.0",
         " * (lang/LEXICON-SOURCES.md). The first four worlds and the first words of each list are BabbleBeat's own. */"]
    for name, ws in worlds:
        o.append(f"static const word W_{name}[]={{")
        for k in range(0, len(ws), 6):
            o.append(" " + ",".join("{" + q(w) + "," + q(p) + "}" for w, p in ws[k:k + 6]) + ",")
        o.append("};")
    o.append("static const struct {const char *name;const word *w;unsigned n;} WORLD[]={")
    o += [f" {{{q(name)},W_{name},{len(ws)}u}}," for name, ws in worlds]
    o.append("};")
    o.append(f"#define BB_WORLDS_N {len(worlds)}")
    for label, ws in (("adj", adj), ("adv", adv), ("prep", prep)):
        o.append(f"static const word {label}[]={{")
        for k in range(0, len(ws), 6):
            o.append(" " + ",".join("{" + q(w) + "," + q(p) + "}" for w, p in ws[k:k + 6]) + ",")
        o.append("};")
    for label, vs in (("intr", intr), ("trans", trans)):
        o.append(f"static const verb {label}[]={{")
        for k in range(0, len(vs), 3):
            o.append(" " + ",".join("{" + ",".join(q(x) for x in v[:4]) + f",{v[4]:#x}u}}" for v in vs[k:k + 3]) + ",")
        o.append("};")
    OUT.write_text("\n".join(o) + "\n")
    nn = sum(len(ws) for _, ws in worlds)
    print(f"{nn} nouns in {len(worlds)} worlds ({', '.join(f'{n} {len(w)}' for n, w in worlds)}); "
          f"{len(adj)} adjectives; {len(adv)} adverbs; {len(prep)} prepositions; {len(trans)} + {len(intr)} verbs; "
          f"{nn + len(adj) + len(adv) + len(prep) + len(trans) + len(intr)} words")


if __name__ == "__main__":
    main()
