yydebug = 0

YYEMPTY = -2
YYEOF = 0
YYTERROR = 1
YYERRCODE = 256



Accept = "Accept"
ParseError = "Parse Error"
NewState = "New State"
Abort = "Abort"
Reduce = "Reduce"
YYDefault = "yydefault"


## ----------------------------------------------------------------------
## YACC PARSER

class BisonParser:
  def __init__(self, module, env=None):
    self.module = module
    self.env = env
    self.yylval = None

    self.lasttoken = len(self.module.yytranslate)
    self.tokenindex = 0
    self.parsetokens = []
    
    self.yypact = self.module.yypact
    self.yytable = self.module.yytable
    self.yycheck = self.module.yycheck

    self.rules = {}

  def yytrans(self, x): 
    if x <= self.lasttoken: return self.module.yytranslate[x]
    else: return self.module.final

  def yylex(self):
    if self.tokenindex >= len(self.parsetokens):
      self.parsetokens = self.tokenize()
      self.tokenindex = 0

    try:
      token, self.lval = self.parsetokens[self.tokenindex]
    except IndexError:
      return YYEOF
    except TypeError:
      token = self.parsetokens[self.tokenindex]
      self.lval = None
  ##  print "new token:", token, yylval, tokenindex, len(parsetokens)

    self.tokenindex = self.tokenindex + 1
    return token
  
  def yyparse(self, fp):
    self.fp = fp

    self.state = 0
    self.n = 0
    self.char = YYEMPTY
    self.char1 = YYEMPTY
    self.errstatus = 0
    self.nerrs = 0

    self.ss = []
    self.vs = []
    self.val = None

    self.len = 0

    while 1:
      try:
	self.yynewstate()
      except NewState:
	pass
      except Reduce:
	self.yyreduce()
      except YYDefault:
	self.yydefault()
      except Accept:
	return self.val


  def yyerrlab(self):

    if not self.errstatus:
      self.nerrs = self.nerrs + 1
      print "parse error"
    self.yyerrlab1()
    return

  def yyerrlab1(self):
  ##  print "yyerrlab1"
    if self.errstatus == 3:
      if self.char == YYEOF:
	raise ParseError
      self.char = YYEMPTY
    self.errstatus = 3
    self.yyerrhandle()

  def yyerrdefault(self):

    self.yyerrpop()

  def yyerrpop(self):
    if len(self.ss) <= 1:
      raise Abort

    self.vs = self.vs[:-1]
    self.ss = self.ss[:-1]

    self.state = self.ss[-1]

    self.yyerrhandle()

  def yyerrhandle(self):
    self.n = self.module.yypact[self.state]
    if self.n == self.module.yyflag:
      self.yyerrdefault()
      return
    self.n = self.n + YYTERROR
    if self.n < 0 or self.n > self.module.yylast or self.module.yycheck[self.n] != YYTERROR:
      self.yyerrdefault()
      return
    self.n = self.module.yytable[self.n]
    if self.n < 0:
      if self.n == self.flag:
	self.yyerrpop()
	return
      self.n = -self.n
      self.yyreduce()
      return
    elif self.n == 0:
      self.yyerrpop()
      return

    if self.n == self.final:
      self.yyaccept()
      return

    self.vs.append(self.lval)
    self.state = self.n
    raise NewState


  def yyaccept(self):
    raise Accept

  def yydefault(self):
    self.n = self.module.yydefact[self.state]
    if self.n == 0:
      print "Error", self.n, self.state
      self.yyerrlab()
      return
    self.yyreduce()

  def yyreduce(self):
    self.len = self.module.yyr2[self.n]
    if self.len > 0:
      self.val = self.vs[-self.len+1]

##    if yydebug:
##      print "Reducing via rule %d (line %d)" % (self.n, self.module.rline[self.n])

    self.val = None

    
    try:
      try:
	rule = self.rules[self.n]
      except KeyError:
	rule = getattr(self.module, "rule_%d" % self.n)
	self.rules[self.n] = rule
    except AttributeError:
      self.rules[self.n] = None
      rule = None

  ##  print

    if rule:
      env= self.env
      if env is None: env= self
      self.val = apply(rule, (self.vs, len(self.vs)-self.len-1, self.lval, env))
    else:
  ##    print "|| default applying", yyn
      if self.len == 1:
	self.val = self.vs[-self.len]
      elif self.len > 0:
	self.val = tuple(self.vs[-self.len:])


  ##  print 'yyval', yyval
  ##  print 'yyvs:', yyvs
  ##  print 'yylen:', yylen

    if self.len:
      self.vs = self.vs[:-self.len]
      self.ss = self.ss[:-self.len]

    self.vs.append(self.val)
  ##  print 'after yyvs:', yyvs

    self.n = self.module.yyr1[self.n]

    self.lyyss = self.ss[-1]


    self.state = self.module.yypgoto[self.n - self.module.yyntbase] + self.lyyss

    if self.state >= 0 and self.state <= self.module.yylast and self.module.yycheck[self.state] == self.lyyss:
      self.state = self.module.yytable[self.state]
    else:
      self.state = self.module.yydefgoto[self.n - self.module.yyntbase]

    ## yynewstate
    return



  def yynewstate(self):
    self.ss.append(self.state)
    self.yybackup()


  def yybackup(self):
    self.n = self.yypact[self.state]
    if self.n == self.module.yyflag:
      raise YYDefault

    if self.char == YYEMPTY:
      self.char = self.yylex()

    if self.char <= 0:
      self.char1 = 0
      self.char = YYEOF
    else:
      self.char1 = self.yytrans(self.char)

##      if yydebug:
##	print "next token is %s(%s)" % (self.char, self.module.yytname[self.char1])

    self.n = self.n + self.char1
    if self.n<0 or self.n > self.module.yylast or self.yycheck[self.n] != self.char1:
      raise YYDefault

    self.n = self.yytable[self.n]

    if self.n < 0:
      if self.n == self.flag:
  ##      print "+++ yyerrlab"
	self.yyerrlab()
	return
      self.n = -self.n

      raise Reduce

    elif self.n == 0:
  ##    print "*** yyerrlab"
      self.yyerrlab()
      return

    if self.n == self.module.yyfinal:
      self.yyaccept()
      return

##    if yydebug:
##      print "Shifting token %d (%s)" % (self.char, self.module.yytname[self.char1]), self.char, self.lval

    if self.char != YYEOF:
      self.char = YYEMPTY

    self.vs.append(self.lval)

    if self.errstatus: self.errstatus = self.errstatus - 1

    self.state = self.n

    return


