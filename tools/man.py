"""man.py - format manual pages written with the -man macros as plain text, for
XENIX's terminals and its `man` command (cmd/man.c), and build the whatis index.

    python3 tools/man.py outdir page.1 page.3 ...

Each page goes to outdir/catN/name.N, formatted for an 80-column screen; outdir/
whatis gets a line for each, "name(N)<tab>names - description", from its NAME
section, sorted by section and name. XENIX 2.1.3 has no nroff, so the pages are
formatted here, the way nroff -man would lay them out in plain text: the macros
.TH .SH .SS .PP .LP .P .TP .IP .HP .RS .RE .PD .nf .fi .br .sp .B .I .BR .BI .IB
.IR .RB .RI .SM .SB, and the escapes \\f \\- \\e \\& \\| \\(xx and \\<space>; others
are ignored. Fonts do not show: the text is plain."""
import os, re, sys

WIDTH = 78          # the right margin
BASE = 5            # where body text starts
TPIND = 7           # the default indent of .TP and .IP
NBSP = '\x01'       # an unpaddable space, until output

SPECIAL = {'em': '--', 'en': '-', 'bu': 'o', 'co': '(c)', 'rg': '(R)', 'aa': "'", 'ga': '`',
           'lq': '"', 'rq': '"', 'mi': '-', 'pl': '+', 'mu': 'x', 'di': '/', 'ge': '>=', 'le': '<=',
           'ne': '!=', '+-': '+-', 'aq': "'", 'dq': '"', 'ti': '~', 'ha': '^', 'rs': '\\', 'ba': '|', 'hy': '-'}

def unescape(s):
    """troff escapes to plain text, in one pass so a \\e stays a backslash"""
    out, i = [], 0
    while i < len(s):
        c = s[i]
        if c != '\\' or i + 1 == len(s):
            out.append(c); i += 1; continue
        e = s[i + 1]; i += 2
        if e == 'f':                            # a font change: \fB, \f(CW
            i += 3 if s[i:i + 1] == '(' else 1
        elif e == '(':                          # a special character: \(em
            out.append(SPECIAL.get(s[i:i + 2], '')); i += 2
        elif e in '&|^c':                       # zero or small spaces, \c
            pass
        elif e == '*':                          # a string: \*R, \*(Tm
            n = s[i + 1:i + 3] if s[i:i + 1] == '(' else s[i:i + 1]
            out.append({'R': '(R)', 'Tm': '(TM)', 'lq': '"', 'rq': '"'}.get(n, ''))
            i += 3 if s[i:i + 1] == '(' else 1
        elif e == ' ':
            out.append(NBSP)
        elif e in 'e\\':
            out.append('\\')
        else:                                   # \- \. \' \` and the rest: the character
            out.append(e)
    return ''.join(out)

def args(line):
    """a macro line's arguments, with "quoted" ones kept together"""
    out, cur, q, i = [], '', False, 0
    while i < len(line):
        c = line[i]
        if c == '\\' and i + 1 < len(line):
            cur += line[i:i + 2]; i += 2; continue
        if c == '"':
            if q and i + 1 < len(line) and line[i + 1] == '"':
                cur += '"'; i += 2; continue
            q = not q
        elif c in ' \t' and not q:
            if cur: out.append(cur)
            cur = ''
        else:
            cur += c
        i += 1
    if cur: out.append(cur)
    return out

class Page:
    def __init__(self):
        self.out = []           # output lines
        self.words = []         # the words being filled
        self.ind = BASE         # the left margin
        self.base = BASE        # where .PP returns to (moved by .RS)
        self.rs = []            # .RS stack
        self.tpind = TPIND
        self.fill = True
        self.tag = None         # a .TP tag waiting for its line
        self.tpnext = False     # the next text line is a .TP tag
        self.first = True       # the paragraph's first line (for .HP)
        self.hang = None
        self.title = ('', '')
        self.names = None
        self.nospace = -1       # no blank line right after a heading (nroff's .ns)
        self.pd = True          # a blank line between paragraphs (.PD 0 turns it off)

    def put(self, line):
        self.out.append(line.replace(NBSP, ' ').rstrip())

    def blank(self):
        if self.out and self.out[-1] != '' and len(self.out) != self.nospace:
            self.out.append('')

    def flush(self):
        """put out the words being filled, after a .TP or .IP tag if one waits"""
        words, self.words = self.words, []
        prefix = None
        if self.tag is not None:
            tag, self.tag = self.tag, None
            if words and len(tag) < self.ind - self.tagind:     # the text fits beside it
                prefix = (' ' * self.tagind + tag).ljust(self.ind)
            else:
                self.put(' ' * self.tagind + tag)
        if not words:
            return
        if prefix is None:
            prefix = ' ' * (self.hang if self.hang is not None else self.ind)
        self.hang = None
        line, empty = prefix, True
        for w in words:
            if not empty and len(line) + 1 + len(w) > WIDTH:
                self.put(line)
                line = ' ' * self.ind + w
            else:
                line += ('' if empty else ' ') + w
            empty = False
        self.put(line)

    def text(self, s):
        s = unescape(s)
        if self.tpnext:
            self.tpnext = False
            self.tag = s.strip()
            return
        if not self.fill:
            self.flush()
            self.put(' ' * self.ind + s.expandtabs(8))    # stops measured from the indent
            return
        if s == '' or s.isspace():
            self.flush(); self.blank(); return
        if s[0] in ' \t':
            self.flush()
        self.words += s.split()

    def para(self):
        self.flush(); self.blank()
        self.ind = self.base
        self.tpind = TPIND

    def macro(self, name, a):
        fonts = {'B', 'I', 'SM', 'SB'}
        alt = {'BR', 'BI', 'IB', 'IR', 'RB', 'RI'}
        if name == 'TH':
            self.title = (unescape(a[0]).upper() if a else '', a[1] if len(a) > 1 else '')
        elif name == 'SH' or name == 'SS':
            self.flush(); self.blank()
            head = unescape(' '.join(a))
            self.put((' ' * 3 if name == 'SS' else '') + head)
            self.nospace = len(self.out)
            self.section = head.upper()
            self.base = self.ind = BASE
            self.rs = []
        elif name in ('PP', 'LP', 'P'):
            self.para()
        elif name in ('TP', 'IP'):
            self.flush()
            if self.pd:
                self.blank()
            n = a[1] if name == 'IP' and len(a) > 1 else a[0] if name == 'TP' and a else None
            if n and re.fullmatch(r'\d+(\.\d+)?[nmi]?', n):
                v = float(re.match(r'[\d.]+', n).group())
                self.tpind = int(v * 10) if n.endswith('i') else int(v)
            self.tagind = self.base
            self.ind = self.base + self.tpind
            if name == 'TP':
                self.tpnext = True
            elif a and a[0]:
                self.tag = unescape(a[0])
        elif name == 'HP':
            self.flush(); self.blank()
            self.ind = self.base + self.tpind
            self.hang = self.base
            self.first = True
        elif name == 'RS':
            self.flush()
            self.rs.append(self.base)
            self.base = self.ind if self.ind > self.base else self.base + self.tpind
            if a and re.fullmatch(r'\d+', a[0]):
                self.base = self.rs[-1] + int(a[0])
            self.ind = self.base
        elif name == 'RE':
            self.flush()
            self.base = self.rs.pop() if self.rs else BASE
            self.ind = self.base
        elif name == 'nf':
            self.flush(); self.fill = False
        elif name == 'fi':
            self.flush(); self.fill = True
        elif name == 'br':
            self.flush()
        elif name == 'PD':
            self.flush()
            self.pd = not (a and a[0] == '0')
        elif name == 'sp':
            self.flush(); self.out.append('')
        elif name in fonts:
            self.text(' '.join(a))
        elif name in alt:
            self.text(''.join(a))
        # anything else (.nh, .ad, .BY, .ne, .UC, .DT ...) has no effect on plain text

    def render(self, src):
        lines = src.split('\n')
        i = 0
        while i < len(lines):
            line = lines[i]; i += 1
            while line.endswith('\\') and not line.endswith('\\\\') and i < len(lines):
                line = line[:-1] + lines[i]; i += 1
            if line.startswith('.\\"') or line.startswith("'\\\"") or line == '.':
                continue
            if line[:1] in ('.', "'"):
                m = re.match(r"[.'](\S+)\s*(.*)", line)
                if not m:
                    continue
                name, rest = m.group(1), m.group(2)
                a = args(rest)
                if name in ('B', 'I', 'SM', 'SB', 'BR', 'BI', 'IB', 'IR', 'RB', 'RI') and not a:
                    if i < len(lines):          # the font applies to the next line
                        a = args(lines[i]); i += 1
                self.macro(name, a)
                if name == 'SH' and unescape(' '.join(a)).upper() == 'NAME':
                    self.names = ''
            else:
                if self.names == '' and line.strip():
                    self.names = unescape(line).strip()
                self.text(line)
        self.flush()
        while self.out and self.out[-1] == '':
            self.out.pop()
        head, sec = self.title
        left = f'{head}({sec})'
        mid = 'xenix86-devsys'
        pad = WIDTH - 2 * len(left) - len(mid)
        top = left + ' ' * (pad // 2) + mid + ' ' * (pad - pad // 2) + left
        return '\n'.join([top, ''] + self.out) + '\n'

def main():
    outdir, pages = sys.argv[1], sys.argv[2:]
    index = []
    for path in pages:
        src = open(path, encoding='latin1').read()
        p = Page()
        text = p.render(src)
        name, sec = os.path.basename(path).rsplit('.', 1)
        os.makedirs(f'{outdir}/cat{sec}', exist_ok=True)
        open(f'{outdir}/cat{sec}/{name}.{sec}', 'w').write(text)
        names = p.names or name
        if ' - ' in names:
            what, desc = names.split(' - ', 1)
        else:
            what, desc = names, ''
        what = ', '.join(w.strip() for w in what.split(','))
        index.append((sec, name, f'{name}({sec})\t{what} - {desc}'.rstrip(' -')))
    index.sort()
    open(f'{outdir}/whatis', 'w').write(''.join(l + '\n' for _, _, l in index))

if __name__ == '__main__':
    main()
