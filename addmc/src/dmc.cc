#include "dmc.hh"

/* global vars ============================================================== */

bool existRandom;
string ddPackage;
bool logCounting;
Float logBound;
string thresholdModel;
bool existPruning;
Int maximizerFormat;
bool maximizerVerification;
bool substitutionMaximization;
Int threadCount;
Int threadSliceCount;
Float memSensitivity;
Float maxMem;
string joinPriority;
Int verboseJoinTree;
Int verboseProfiling;

Int dotFileIndex = 1;

/* classes for processing join trees ======================================== */

/* class JoinTree =========================================================== */

JoinNode* JoinTree::getJoinNode(Int nodeIndex) const {
  if (joinTerminals.contains(nodeIndex)) {
    return joinTerminals.at(nodeIndex);
  }
  return joinNonterminals.at(nodeIndex);
}

JoinNonterminal* JoinTree::getJoinRoot() const {
  return joinNonterminals.at(declaredNodeCount - 1);
}

void JoinTree::printTree() const {
  cout << "c p " << JOIN_TREE_WORD << " " << declaredVarCount << " " << declaredClauseCount << " " << declaredNodeCount << "\n";
  getJoinRoot()->printSubtree("c ");
}

JoinTree::JoinTree(Int declaredVarCount, Int declaredClauseCount, Int declaredNodeCount) {
  this->declaredVarCount = declaredVarCount;
  this->declaredClauseCount = declaredClauseCount;
  this->declaredNodeCount = declaredNodeCount;
}

/* class JoinTreeProcessor ================================================== */

Int JoinTreeProcessor::plannerPid = MIN_INT;
JoinTree* JoinTreeProcessor::joinTree = nullptr;
JoinTree* JoinTreeProcessor::backupJoinTree = nullptr;

void JoinTreeProcessor::killPlanner() {
  if (plannerPid == MIN_INT) {
    cout << WARNING << "found no pid for planner process\n";
  }
  else if (kill(plannerPid, SIGKILL) == 0) {
    cout << "c killed planner process with pid " << plannerPid << "\n";
  }
  else {
    // cout << WARNING << "failed to kill planner process with pid " << plannerPid << "\n";
  }
}

void JoinTreeProcessor::handleSigAlrm(int signal) {
  assert(signal == SIGALRM);
  cout << "c received SIGALRM after " << util::getDuration(toolStartPoint) << "s\n";

  if (joinTree == nullptr && backupJoinTree == nullptr) {
    cout << "c found no join tree yet; will wait for first join tree then kill planner\n";
  }
  else {
    cout << "c found join tree; killing planner\n";
    killPlanner();
  }
}

bool JoinTreeProcessor::hasDisarmedTimer() {
  struct itimerval curr_value;
  getitimer(ITIMER_REAL, &curr_value);

  return curr_value.it_value.tv_sec == 0 && curr_value.it_value.tv_usec == 0 && curr_value.it_interval.tv_sec == 0 && curr_value.it_interval.tv_usec == 0;
}

void JoinTreeProcessor::setTimer(Float seconds) {
  assert(seconds >= 0);

  Int secs = seconds;
  Int usecs = (seconds - secs) * 1000000;

  struct itimerval new_value;
  new_value.it_value.tv_sec = secs;
  new_value.it_value.tv_usec = usecs;
  new_value.it_interval.tv_sec = 0;
  new_value.it_interval.tv_usec = 0;

  setitimer(ITIMER_REAL, &new_value, nullptr);
}

void JoinTreeProcessor::armTimer(Float seconds) {
  assert(seconds >= 0);
  signal(SIGALRM, handleSigAlrm);
  setTimer(seconds);
}

void JoinTreeProcessor::disarmTimer() {
  setTimer(0);
  cout << "c disarmed timer\n";
}

const JoinNonterminal* JoinTreeProcessor::getJoinTreeRoot() const {
  return joinTree->getJoinRoot();
}

void JoinTreeProcessor::processCommentLine(const vector<string>& words) {
  if (words.size() == 3) {
    string key = words.at(1);
    string val = words.at(2);
    if (key == "pid") {
      plannerPid = stoll(val);
    }
    else if (key == "joinTreeWidth") {
      if (joinTree != nullptr) {
        joinTree->width = stoll(val);
      }
    }
    else if (key == "seconds") {
      if (joinTree != nullptr) {
        joinTree->plannerDuration = stold(val);
      }
    }
  }
}

void JoinTreeProcessor::processProblemLine(const vector<string>& words) {
  if (problemLineIndex != MIN_INT) {
    throw MyError("multiple problem lines: ", problemLineIndex, " and ", lineIndex);
  }
  problemLineIndex = lineIndex;

  if (words.size() != 5) {
    throw MyError("problem line ", lineIndex, " has ", words.size(), " words (should be 5)");
  }

  string jtWord = words.at(1);
  if (jtWord != JOIN_TREE_WORD) {
    throw MyError("expected '", JOIN_TREE_WORD, "'; found '", jtWord, "' | line ", lineIndex);
  }

  Int declaredVarCount = stoll(words.at(2));
  Int declaredClauseCount = stoll(words.at(3));
  Int declaredNodeCount = stoll(words.at(4));

  joinTree = new JoinTree(declaredVarCount, declaredClauseCount, declaredNodeCount);

  for (Int terminalIndex = 0; terminalIndex < declaredClauseCount; terminalIndex++) {
    joinTree->joinTerminals[terminalIndex] = new JoinTerminal();
  }
}

void JoinTreeProcessor::processNonterminalLine(const vector<string>& words) {
  if (problemLineIndex == MIN_INT) {
    string message = "no problem line before internal node | line " + to_string(lineIndex);
    if (joinTreeEndLineIndex != MIN_INT) {
      message += " (previous join tree ends on line " + to_string(joinTreeEndLineIndex) + ")";
    }
    throw MyError(message);
  }

  Int parentIndex = stoll(words.front()) - 1; // 0-indexing
  if (parentIndex < joinTree->declaredClauseCount || parentIndex >= joinTree->declaredNodeCount) {
    throw MyError("wrong internal-node index | line ", lineIndex);
  }

  vector<JoinNode*> children;
  Set<Int> projectionVars;
  bool parsingElimVars = false;
  for (Int i = 1; i < words.size(); i++) {
    string word = words.at(i);
    if (word == ELIM_VARS_WORD) {
      parsingElimVars = true;
    }
    else {
      Int num = stoll(word);
      if (parsingElimVars) {
        Int declaredVarCount = joinTree->declaredVarCount;
        if (num <= 0 || num > declaredVarCount) {
          throw MyError("var '", num, "' inconsistent with declared var count '", declaredVarCount, "' | line ", lineIndex);
        }
        projectionVars.insert(num);
      }
      else {
        Int childIndex = num - 1; // 0-indexing
        if (childIndex < 0 || childIndex >= parentIndex) {
          throw MyError("child '", word, "' wrong | line ", lineIndex);
        }
        children.push_back(joinTree->getJoinNode(childIndex));
      }
    }
  }
  joinTree->joinNonterminals[parentIndex] = new JoinNonterminal(children, projectionVars, parentIndex);
}

void JoinTreeProcessor::finishReadingJoinTree() {
  Int nonterminalCount = joinTree->joinNonterminals.size();
  Int expectedNonterminalCount = joinTree->declaredNodeCount - joinTree->declaredClauseCount;

  if (nonterminalCount < expectedNonterminalCount) {
    cout << WARNING << "missing internal nodes (" << expectedNonterminalCount << " expected, " << nonterminalCount << " found) before current join tree ends on line " << lineIndex << "\n";
  }
  else {
    if (joinTree->width == MIN_INT) {
      joinTree->width = joinTree->getJoinRoot()->getWidth();
    }

    cout << "c processed join tree ending on line " << lineIndex << "\n";
    util::printRow("joinTreeWidth", joinTree->width);
    util::printRow("plannerSeconds", joinTree->plannerDuration);

    if (verboseJoinTree >= PARSED_INPUT) {
      cout << DASH_LINE;
      joinTree->printTree();
      cout << DASH_LINE;
    }

    joinTreeEndLineIndex = lineIndex;
    backupJoinTree = joinTree;
    JoinNode::resetStaticFields();
  }

  problemLineIndex = MIN_INT;
  joinTree = nullptr;
}

void JoinTreeProcessor::readInputStream() {
  string line;
  while (getline(std::cin, line)) {
    lineIndex++;

    if (verboseJoinTree >= RAW_INPUT) {
      util::printInputLine(line, lineIndex);
    }

    vector<string> words = util::splitInputLine(line);
    if (words.empty()) {}
    else if (words.front() == "=") { // LG's tree separator "="
      if (joinTree != nullptr) {
        finishReadingJoinTree();
      }
      if (hasDisarmedTimer()) { // timer expires before first join tree ends
        break;
      }
    }
    else if (words.front() == "c") { // possibly special comment line
      processCommentLine(words);
    }
    else if (words.front() == "p") { // problem line
      processProblemLine(words);
    }
    else { // nonterminal-node line
      processNonterminalLine(words);
    }
  }

  if (joinTree != nullptr) {
    finishReadingJoinTree();
  }

  if (!hasDisarmedTimer()) { // stdin ends before timer expires
    cout << "c stdin ends before timer expires; disarming timer\n";
    disarmTimer();
  }
}

JoinTreeProcessor::JoinTreeProcessor(Float plannerWaitDuration) {
  cout << "c procressing join tree...\n";

  armTimer(plannerWaitDuration);
  cout << "c getting join tree from stdin with " << plannerWaitDuration << "s timer (end input with 'enter' then 'ctrl d')\n";

  readInputStream();

  if (joinTree == nullptr) {
    if (backupJoinTree == nullptr) {
      throw MyError("no join tree before line ", lineIndex);
    }
    joinTree = backupJoinTree;
    JoinNode::restoreStaticFields();
  }

  cout << "c getting join tree from stdin: done\n";

  if (plannerPid != MIN_INT) { // timer expires before first join tree ends
    killPlanner();
  }
}

/* classes for execution ==================================================== */

/* class SatSolver ========================================================== */

bool SatSolver::checkSat(bool exceptionThrowing) {
  lbool satisfiability = cmsat.solve();
  if (satisfiability == l_False) {
    if (exceptionThrowing) {
      throw UnsatSolverException();
    }
    return false;
  }
  assert(satisfiability == l_True);
  return true;
}

Assignment SatSolver::getModel() {
  vector<Lit> banLits;
  Assignment model;
  vector<lbool> lbools = cmsat.get_model();
  for (Int i = 0; i < lbools.size(); i++) {
    Int cnfVar = i + 1;
    bool val = true;
    lbool b = lbools.at(i);
    if (b != CMSat::l_Undef) {
      assert(b == l_True || b == l_False);
      val = b == l_True;
      banLits.push_back(getLit(cnfVar, !val));
    }
    cmsat.add_clause(banLits);
    model[cnfVar] = val;
  }
  return model;
}

Lit SatSolver::getLit(Int cnfVar, bool val) {
  assert(cnfVar > 0);
  return Lit(cnfVar - 1, !val);
}

SatSolver::SatSolver(const Cnf& cnf) {
  cmsat.new_vars(cnf.declaredVarCount);
  for (const Clause& clause : cnf.clauses) {
    if (clause.xorFlag) {
      vector<unsigned> vars;
      bool rhs = true;
      for (Int literal : clause) {
        vars.push_back(abs(literal) - 1);
        if (literal < 0) {
          rhs = !rhs;
        }
      }
      cmsat.add_xor_clause(vars, rhs);
    }
    else {
      vector<Lit> lits;
      for (Int literal : clause) {
        lits.push_back(getLit(abs(literal), literal > 0));
      }
      cmsat.add_clause(lits);
    }
  }
}

/* class Dd ================================================================= */

Float Dd::pruningDuration;

Dd::Dd(const ADD& cuadd) {
  assert(ddPackage == CUDD);
  this->cuadd = cuadd;
}

Dd::Dd(const Mtbdd& mtbdd) {
  assert(ddPackage == SYLVAN);
  this->mtbdd = mtbdd;
}

Dd::Dd(const Dd& dd) {
  if (ddPackage == CUDD) {
    this->cuadd = dd.cuadd;
  }
  else {
    this->mtbdd = dd.mtbdd;
  }
}

Number Dd::extractConst() const {
  if (ddPackage == CUDD) {
    ADD minTerminal = cuadd.FindMin();
    assert(minTerminal == cuadd.FindMax());
    return Number(cuddV(minTerminal.getNode()));
  }
  assert(mtbdd.isLeaf());
  if (multiplePrecision) {
    return Number(mpq_class((mpq_ptr)mtbdd_getvalue(mtbdd.GetMTBDD())));
  }
  return Number(mtbdd_getdouble(mtbdd.GetMTBDD()));
}

Dd Dd::getConstDd(const Number& n, const Cudd* mgr) {
  if (ddPackage == CUDD) {
    return logCounting ? Dd(mgr->constant(n.getLog10())) : Dd(mgr->constant(n.fraction));
  }
  if (multiplePrecision) {
    mpq_t q; // C interface
    mpq_init(q);
    mpq_set(q, n.quotient.get_mpq_t());
    Dd dd(Mtbdd(mtbdd_gmp(q)));
    mpq_clear(q);
    return dd;
  }
  return Dd(Mtbdd::doubleTerminal(n.fraction));
}

Dd Dd::getZeroDd(const Cudd* mgr) {
  return getConstDd(Number(), mgr);
}

Dd Dd::getOneDd(const Cudd* mgr) {
  return getConstDd(Number("1"), mgr);
}

Dd Dd::getVarDd(Int ddVar, bool val, const Cudd* mgr) {
  if (ddPackage == CUDD) {
    if (logCounting) {
      return Dd(mgr->addLogVar(ddVar, val));
    }
    ADD d = mgr->addVar(ddVar);
    return val ? Dd(d) : Dd(d.Cmpl());
  }
  MTBDD d0 = getZeroDd(mgr).mtbdd.GetMTBDD();
  MTBDD d1= getOneDd(mgr).mtbdd.GetMTBDD();
  return val ? Dd(mtbdd_makenode(ddVar, d0, d1)) : Dd(mtbdd_makenode(ddVar, d1, d0));
}

const Cudd* Dd::newMgr(Float mem, Int threadIndex) {
  assert(ddPackage == CUDD);
  Cudd* mgr = new Cudd(
    0, // init num of BDD vars
    0, // init num of ZDD vars
    CUDD_UNIQUE_SLOTS, // init num of unique-table slots; cudd.h: #define CUDD_UNIQUE_SLOTS 256
    CUDD_CACHE_SLOTS, // init num of cache-table slots; cudd.h: #define CUDD_CACHE_SLOTS 262144
    mem * MEGA // maxMemory
  );
  mgr->getManager()->threadIndex = threadIndex;
  mgr->getManager()->peakMemIncSensitivity = memSensitivity * MEGA; // makes CUDD print "c cuddMegabytes_{threadIndex + 1} {memused / 1e6}"
  if (verboseSolving >= 3 && threadIndex == 0) {
    // util::printRow("hardMaxMemMegabytes", mgr->ReadMaxMemory() / MEGA); // for unique table and cache table combined (unlimited by default)
    // util::printRow("softMaxMemMegabytes", mgr->getManager()->maxmem / MEGA); // cuddInt.c: maxmem = maxMemory / 10 * 9
    // util::printRow("hardMaxCacheMegabytes", mgr->ReadMaxCacheHard() * sizeof(DdCache) / MEGA); // cuddInt.h: #define DD_MAX_CACHE_FRACTION 3
    // writeInfoFile(mgr, "info.txt");
  }
  return mgr;
}

size_t Dd::getNodeCount() const {
  if (ddPackage == CUDD) {
    return cuadd.nodeCount();
  }
  return mtbdd.NodeCount();
}

bool Dd::operator!=(const Dd& rightDd) const {
  if (ddPackage == CUDD) {
    return cuadd != rightDd.cuadd;
  }
  return mtbdd != rightDd.mtbdd;
}

bool Dd::operator<(const Dd& rightDd) const {
  if (joinPriority == SMALLEST_PAIR) { // top = rightmost = smallest
    return getNodeCount() > rightDd.getNodeCount();
  }
  return getNodeCount() < rightDd.getNodeCount();
}

Dd Dd::getComposition(Int ddVar, bool val, const Cudd* mgr) const {
  if (ddPackage == CUDD) {
    if (util::isFound(ddVar, cuadd.SupportIndices())) {
      return Dd(cuadd.Compose(val ? mgr->addOne() : mgr->addZero(), ddVar));
    }
    return *this;
  }
  sylvan::MtbddMap m;
  m.put(ddVar, val ? Mtbdd::mtbddOne() : Mtbdd::mtbddZero());
  return Dd(mtbdd.Compose(m));
}

Dd Dd::getProduct(const Dd& dd) const {
  if (ddPackage == CUDD) {
    return logCounting ? Dd(cuadd + dd.cuadd) : Dd(cuadd * dd.cuadd);
  }
  if (multiplePrecision) {
    LACE_ME;
    return Dd(Mtbdd(gmp_times(mtbdd.GetMTBDD(), dd.mtbdd.GetMTBDD())));
  }
  return Dd(mtbdd * dd.mtbdd);
}

Dd Dd::getSum(const Dd& dd) const {
  if (ddPackage == CUDD) {
    return logCounting ? Dd(cuadd.LogSumExp(dd.cuadd)) : Dd(cuadd + dd.cuadd);
  }
  if (multiplePrecision) {
    LACE_ME;
    return Dd(Mtbdd(gmp_plus(mtbdd.GetMTBDD(), dd.mtbdd.GetMTBDD())));
  }
  return Dd(mtbdd + dd.mtbdd);
}

Dd Dd::getMax(const Dd& dd) const {
  if (ddPackage == CUDD) {
    return Dd(cuadd.Maximum(dd.cuadd));
  }
  if (multiplePrecision) {
    LACE_ME;
    return Dd(Mtbdd(gmp_max(mtbdd.GetMTBDD(), dd.mtbdd.GetMTBDD())));
  }
  return Dd(mtbdd.Max(dd.mtbdd));
}

Dd Dd::getXor(const Dd& dd) const {
  assert(ddPackage == CUDD);
  return logCounting ? Dd(cuadd.LogXor(dd.cuadd)) : Dd(cuadd.Xor(dd.cuadd));
}

Set<Int> Dd::getSupport() const {
  Set<Int> support;
  if (ddPackage == CUDD) {
    for (Int ddVar : cuadd.SupportIndices()) {
      support.insert(ddVar);
    }
  }
  else {
    Mtbdd cube = mtbdd.Support(); // conjunction of all vars appearing in mtbdd
    while (!cube.isOne()) {
      support.insert(cube.TopVar());
      cube = cube.Then();
    }
  }
  return support;
}

Dd Dd::getBoolDiff(const Dd& rightDd) const {
  assert(ddPackage == CUDD);
  return Dd((cuadd - rightDd.cuadd).BddThreshold(0).Add());
}

bool Dd::evalAssignment(vector<int>& ddVarAssignment) const {
  assert(ddPackage == CUDD);
  Number n = Dd(cuadd.Eval(&ddVarAssignment.front())).extractConst();
  return n == Number("1");
}

Dd Dd::getAbstraction(Int ddVar, const vector<Int>& ddVarToCnfVarMap, const Map<Int, Number>& literalWeights, const Assignment& assignment, bool additiveFlag, vector<pair<Int, Dd>>& maximizationStack, const Cudd* mgr) const {
  Int cnfVar = ddVarToCnfVarMap.at(ddVar);
  Dd positiveWeight = getConstDd(literalWeights.at(cnfVar), mgr);
  Dd negativeWeight = getConstDd(literalWeights.at(-cnfVar), mgr);

  auto it = assignment.find(cnfVar);
  if (it != assignment.end()) {
    Dd weight = it->second ? positiveWeight : negativeWeight;
    return getProduct(weight);
  }

  Dd highTerm = getComposition(ddVar, true, mgr).getProduct(positiveWeight);
  Dd lowTerm = getComposition(ddVar, false, mgr).getProduct(negativeWeight);

  if (maximizerFormat && !additiveFlag) {
    Dd dsgn = highTerm.getBoolDiff(lowTerm); // derivative sign
    maximizationStack.push_back({ddVar, dsgn});
    if (substitutionMaximization) {
      return Dd(cuadd.Compose(dsgn.cuadd, ddVar));
    }
  }

  return additiveFlag ? highTerm.getSum(lowTerm) : highTerm.getMax(lowTerm);
}

Dd Dd::getPrunedDd(Float lowerBound, const Cudd* mgr) const {
  assert(logCounting);

  TimePoint pruningStartPoint = util::getTimePoint();

  ADD bound = mgr->constant(lowerBound);
  ADD prunedDd = cuadd.LogThreshold(bound);

  pruningDuration += util::getDuration(pruningStartPoint);

  return Dd(prunedDd);
}

void Dd::writeDotFile(const Cudd* mgr, string dotFileDir) const {
  string filePath = dotFileDir + "dd" + to_string(dotFileIndex++) + ".dot";
  FILE* file = fopen(filePath.c_str(), "wb"); // writes to binary file

  if (ddPackage == CUDD) { // davidkebo.com/cudd#cudd6
    DdNode** ddNodeArray = static_cast<DdNode**>(malloc(sizeof(DdNode*)));
    ddNodeArray[0] = cuadd.getNode();
    Cudd_DumpDot(mgr->getManager(), 1, ddNodeArray, NULL, NULL, file);
    free(ddNodeArray);
  }
  else {
    mtbdd_fprintdot_nc(file, mtbdd.GetMTBDD());
  }

  fclose(file);
  cout << "c wrote decision diagram to file " << filePath << "\n";
}

void Dd::writeInfoFile(const Cudd* mgr, string filePath) {
  assert(ddPackage == CUDD);
  FILE* file = fopen(filePath.c_str(), "w");
  Cudd_PrintInfo(mgr->getManager(), file);
  fclose(file);
  cout << "c wrote CUDD info to file " << filePath << "\n";
}

/* class Executor =========================================================== */

vector<pair<Int, Dd>> Executor::maximizationStack;
Int Executor::prunedDdCount;

Map<Int, Float> Executor::varDurations;
Map<Int, size_t> Executor::varDdSizes;

void Executor::updateVarDurations(const JoinNode* joinNode, TimePoint startPoint) {
  if (verboseProfiling >= 1) {
    Float duration = util::getDuration(startPoint);
    if (duration > 0) {
      if (verboseProfiling >= 2) {
        util::printRow("joinNodeSeconds_" + to_string(joinNode->nodeIndex + 1), duration);
      }

      for (Int var : joinNode->preProjectionVars) {
        if (varDurations.contains(var)) {
          varDurations[var] += duration;
        }
        else {
          varDurations[var] = duration;
        }
      }
    }
  }
}

void Executor::updateVarDdSizes(const JoinNode* joinNode, const Dd& dd) {
  if (verboseProfiling >= 1) {
    size_t ddSize = dd.getNodeCount();

    if (verboseProfiling >= 2) {
      util::printRow("joinNodeDiagramSize_" + to_string(joinNode->nodeIndex + 1), ddSize);
    }

    for (Int var : joinNode->preProjectionVars) {
      if (varDdSizes.contains(var)) {
        varDdSizes[var] = max(varDdSizes[var], ddSize);
      }
      else {
        varDdSizes[var] = ddSize;
      }
    }
  }
}

void Executor::printVarDurations() {
  multimap<Float, Int, greater<Float>> timedVars = util::flipMap(varDurations); // duration |-> var
  for (pair<Float, Int> timedVar : timedVars) {
    util::printRow("varTotalSeconds_" + to_string(timedVar.second), timedVar.first);
  }
}

void Executor::printVarDdSizes() {
  multimap<size_t, Int, greater<size_t>> sizedVars = util::flipMap(varDdSizes); // DD size |-> var
  for (pair<size_t, Int> sizedVar : sizedVars) {
    util::printRow("varMaxDiagramSize_" + to_string(sizedVar.second), sizedVar.first);
  }
}

Dd Executor::getClauseDd(const Map<Int, Int>& cnfVarToDdVarMap, const Clause& clause, const Cudd* mgr, const Assignment& assignment) {
  Dd clauseDd = Dd::getZeroDd(mgr);
  for (Int literal : clause) {
    bool val = literal > 0;
    Int cnfVar = abs(literal);
    auto it = assignment.find(cnfVar);
    if (it != assignment.end()) { // literal has assigned value
      if (it->second == val) {
        if (clause.xorFlag) { // flips polarity
          clauseDd = clauseDd.getXor(Dd::getOneDd(mgr));
        }
        else { // returns satisfied disjunctive clause
          return Dd::getOneDd(mgr);
        }
      } // excludes unsatisfied literal from clause otherwise
    }
    else { // literal is unassigned
      Int ddVar = cnfVarToDdVarMap.at(cnfVar);
      Dd literalDd = Dd::getVarDd(ddVar, val, mgr);
      clauseDd = clause.xorFlag ? clauseDd.getXor(literalDd) : clauseDd.getMax(literalDd);
    }
  }
  return clauseDd;
}

Dd Executor::solveSubtree(const JoinNode* joinNode, const Map<Int, Int>& cnfVarToDdVarMap, const vector<Int>& ddVarToCnfVarMap, const Cudd* mgr, const Assignment& assignment) {
  if (joinNode->isTerminal()) {
    TimePoint terminalStartPoint = util::getTimePoint();

    Dd d = getClauseDd(cnfVarToDdVarMap, JoinNode::cnf.clauses.at(joinNode->nodeIndex), mgr, assignment);

    updateVarDurations(joinNode, terminalStartPoint);
    updateVarDdSizes(joinNode, d);

    return d;
  }

  vector<Dd> childDdList;
  for (JoinNode* child : joinNode->children) {
    childDdList.push_back(solveSubtree(child, cnfVarToDdVarMap, ddVarToCnfVarMap, mgr, assignment));
  }

  TimePoint nonterminalStartPoint = util::getTimePoint();
  Dd dd = Dd::getOneDd(mgr);

  if (joinPriority == ARBITRARY_PAIR) { // arbitrarily multiplies child decision diagrams
    for (Dd childDd : childDdList) {
      dd = dd.getProduct(childDd);
    }
  }
  else { // Dd::operator< handles both biggest-first and smallest-first
    std::priority_queue<Dd> childDdQueue;
    for (Dd childDd : childDdList) {
      childDdQueue.push(childDd);
    }
    assert(!childDdQueue.empty());
    while (childDdQueue.size() >= 2) {
      Dd dd1 = childDdQueue.top();
      childDdQueue.pop();
      Dd dd2 = childDdQueue.top();
      childDdQueue.pop();
      Dd dd3 = dd1.getProduct(dd2);
      childDdQueue.push(dd3);
    }
    dd = childDdQueue.top();
  }

  for (Int cnfVar : joinNode->projectionVars) {
    Int ddVar = cnfVarToDdVarMap.at(cnfVar);

    bool additiveFlag = JoinNode::cnf.outerVars.contains(cnfVar);
    if (existRandom) {
      additiveFlag = !additiveFlag;
    }

    dd = dd.getAbstraction(ddVar, ddVarToCnfVarMap, JoinNode::cnf.literalWeights, assignment, additiveFlag, maximizationStack, mgr);

    if (logBound > -INF) {
      if (JoinNode::cnf.literalWeights.at(cnfVar) < Number(1) || JoinNode::cnf.literalWeights.at(-cnfVar) < Number(1)) {
        Dd prunedDd = dd.getPrunedDd(logBound, mgr);
        if (prunedDd != dd) {
          if (verboseSolving >= 3) {
            cout << "c writing pre-pruning decision diagram...\n";
            dd.writeDotFile(mgr);

            cout << "c writing post-pruning decision diagram...\n";
            prunedDd.writeDotFile(mgr);
          }
          prunedDdCount++;
          dd = prunedDd;
        }
      }
    }
  }

  updateVarDurations(joinNode, nonterminalStartPoint);
  updateVarDdSizes(joinNode, dd);

  return dd;
}

void Executor::solveThreadSlices(const JoinNonterminal* joinRoot, const Map<Int, Int>& cnfVarToDdVarMap, const vector<Int>& ddVarToCnfVarMap, Float threadMem, Int threadIndex, const vector<vector<Assignment>>& threadAssignmentLists, Number& totalSolution, mutex& solutionMutex) {
  const vector<Assignment>& threadAssignments = threadAssignmentLists.at(threadIndex);
  for (Int threadAssignmentIndex = 0; threadAssignmentIndex < threadAssignments.size(); threadAssignmentIndex++) {
    TimePoint sliceStartPoint = util::getTimePoint();

    Number partialSolution = solveSubtree(static_cast<const JoinNode*>(joinRoot), cnfVarToDdVarMap, ddVarToCnfVarMap, Dd::newMgr(threadMem, threadIndex), threadAssignments.at(threadAssignmentIndex)).extractConst();

    const std::lock_guard<mutex> g(solutionMutex);

    if (verboseSolving >= 1) {
      cout << "c thread " << right << setw(4) << threadIndex + 1 << "/" << threadAssignmentLists.size();
      cout << " | assignment " << setw(4) << threadAssignmentIndex + 1 << "/" << threadAssignments.size();

      cout << ": { ";
      threadAssignments.at(threadAssignmentIndex).printAssignment();
      cout << " }\n";

      cout << "c thread " << right << setw(4) << threadIndex + 1 << "/" << threadAssignmentLists.size();
      cout << " | assignment " << setw(4) << threadAssignmentIndex + 1 << "/" << threadAssignments.size();
      cout << " | seconds " << std::fixed << setw(10) << util::getDuration(sliceStartPoint);
      cout << " | solution " << setw(15) << partialSolution << "\n";
    }

    if (existRandom) {
      totalSolution = max(totalSolution, partialSolution);
    }
    else {
      totalSolution = logCounting ? Number(totalSolution.getLogSumExp(partialSolution)) : totalSolution + partialSolution;
    }
  }
}

vector<vector<Assignment>> Executor::getThreadAssignmentLists(const JoinNonterminal* joinRoot, Int sliceVarOrderHeuristic) {
  size_t sliceVarCount = ceill(log2l(threadCount * threadSliceCount));
  sliceVarCount = min(sliceVarCount, JoinNode::cnf.outerVars.size());

  Int remainingSliceCount = exp2l(sliceVarCount);
  Int remainingThreadCount = threadCount;
  vector<Int> threadSliceCounts;
  while (remainingThreadCount > 0) {
    Int sliceCount = ceill(static_cast<Float>(remainingSliceCount) / remainingThreadCount);
    threadSliceCounts.push_back(sliceCount);
    remainingSliceCount -= sliceCount;
    remainingThreadCount--;
  }
  assert(remainingSliceCount == 0);

  if (verboseSolving >= 1) {
    cout << "c thread slice counts: { ";
    for (Int sliceCount : threadSliceCounts) {
      cout << sliceCount << " ";
    }
    cout << "}\n";
  }

  vector<Assignment> assignments = joinRoot->getOuterAssignments(sliceVarOrderHeuristic, sliceVarCount);
  vector<vector<Assignment>> threadAssignmentLists;
  vector<Assignment> threadAssignmentList;
  for (Int assignmentIndex = 0, threadListIndex = 0; assignmentIndex < assignments.size() && threadListIndex < threadSliceCounts.size(); assignmentIndex++) {
    threadAssignmentList.push_back(assignments.at(assignmentIndex));
    if (threadAssignmentList.size() == threadSliceCounts.at(threadListIndex)) {
      threadAssignmentLists.push_back(threadAssignmentList);
      threadAssignmentList.clear();
      threadListIndex++;
    }
  }

  if (verboseSolving >= 2) {
    for (Int threadIndex = 0; threadIndex < threadAssignmentLists.size(); threadIndex++) {
      const vector<Assignment>& threadAssignments = threadAssignmentLists.at(threadIndex);
      cout << "c assignments in thread " << right << setw(4) << threadIndex + 1 << ":";
      for (const Assignment& assignment : threadAssignments) {
        cout << " { ";
        assignment.printAssignment();
        cout << " }";
      }
      cout << "\n";
    }
  }

  return threadAssignmentLists;
}

Number Executor::solveCnf(const JoinNonterminal* joinRoot, const Map<Int, Int>& cnfVarToDdVarMap, const vector<Int>& ddVarToCnfVarMap, Int sliceVarOrderHeuristic) {
  if (ddPackage == SYLVAN) {
    return solveSubtree(
      static_cast<const JoinNode*>(joinRoot),
      cnfVarToDdVarMap,
      ddVarToCnfVarMap
    ).extractConst();
  }

  vector<vector<Assignment>> threadAssignmentLists = getThreadAssignmentLists(joinRoot, sliceVarOrderHeuristic);
  util::printRow("sliceWidth", joinRoot->getWidth(threadAssignmentLists.front().front())); // any assignment would work
  Number totalSolution = logCounting ? Number(-INF) : Number();
  mutex solutionMutex;

  Float threadMem = maxMem / threadAssignmentLists.size();
  util::printRow("threadMaxMemMegabytes", threadMem);

  vector<thread> threads;

  Int threadIndex = 0;
  for (; threadIndex < threadAssignmentLists.size() - 1; threadIndex++) {
    threads.push_back(thread(
      solveThreadSlices,
      std::cref(joinRoot),
      std::cref(cnfVarToDdVarMap),
      std::cref(ddVarToCnfVarMap),
      threadMem,
      threadIndex,
      threadAssignmentLists,
      std::ref(totalSolution),
      std::ref(solutionMutex)
    ));
  }
  solveThreadSlices(
    joinRoot,
    cnfVarToDdVarMap,
    ddVarToCnfVarMap,
    threadMem,
    threadIndex,
    threadAssignmentLists,
    totalSolution,
    solutionMutex
  );
  for (thread& t : threads) {
    t.join();
  }

  return totalSolution;
}

void Executor::setLogBound(const JoinNonterminal* joinRoot, const Map<Int, Int>& cnfVarToDdVarMap, const vector<Int>& ddVarToCnfVarMap) {
  if (logBound > -INF) {} // LOG_BOUND_OPTION
  else if (!thresholdModel.empty()) { // THRESHOLD_MODEL_OPTION
    logBound = solveSubtree(
      joinRoot,
      cnfVarToDdVarMap,
      ddVarToCnfVarMap,
      Dd::newMgr(maxMem),
      Assignment(thresholdModel)
    ).extractConst().fraction;
    util::printRow("logBound", logBound);
  }
  else if (existPruning) { // EXIST_PRUNING_OPTION
    SatSolver satSolver(joinRoot->cnf);
    satSolver.checkSat(true);
    Assignment model = satSolver.getModel();
    logBound = solveSubtree(
      joinRoot,
      cnfVarToDdVarMap,
      ddVarToCnfVarMap,
      Dd::newMgr(maxMem),
      model
    ).extractConst().fraction;
    util::printRow("logBound", logBound);
    cout << "c " << getShortModel(model, joinRoot->cnf.declaredVarCount) << "\n";
  }
}

Number Executor::adjustSolutionToHiddenVar(const Number &apparentSolution, Int cnfVar, bool additiveFlag) {
  if (JoinNode::cnf.apparentVars.contains(cnfVar)) {
    return apparentSolution;
  }

  const Number& positiveWeight = JoinNode::cnf.literalWeights.at(cnfVar);
  const Number& negativeWeight = JoinNode::cnf.literalWeights.at(-cnfVar);
  if (additiveFlag) {
    return logCounting ? apparentSolution + (positiveWeight + negativeWeight).getLog10() : apparentSolution * (positiveWeight + negativeWeight);
  }
  else {
    return logCounting ? apparentSolution + max(positiveWeight, negativeWeight).getLog10(): apparentSolution * max(positiveWeight, negativeWeight); // positive weights
  }
}

Number Executor::getAdjustedSolution(const Number &apparentSolution) {
  Number n = apparentSolution;

  for (Int var = 1; var <= JoinNode::cnf.declaredVarCount; var++) { // processes inner vars
    if (!JoinNode::cnf.outerVars.contains(var)) {
      n = adjustSolutionToHiddenVar(apparentSolution, var, existRandom);
    }
  }

  for (Int var : JoinNode::cnf.outerVars) {
    n = adjustSolutionToHiddenVar(apparentSolution, var, !existRandom);
  }

  return n;
}

void Executor::printSatRow(const Number& solution, bool unsatFlag, size_t keyWidth) {
  const string SAT_WORD = "SATISFIABLE";
  const string UNSAT_WORD = "UN" + SAT_WORD;

  string satisfiability = "UNKNOWN";

  if (unsatFlag) {
    satisfiability = UNSAT_WORD;
  }
  else if (existPruning) {
    satisfiability = SAT_WORD; // otherwise, UnsatSolverException would have been thrown earlier
  }
  else if (logCounting) {
    if (solution == Number(-INF)) {
      if (!weightedCounting) {
        satisfiability = UNSAT_WORD;
      }
    }
    else {
      satisfiability = SAT_WORD;
    }
  }
  else {
    if (solution == Number()) {
      if (!weightedCounting || multiplePrecision) {
        satisfiability = UNSAT_WORD;
      }
    }
    else {
      satisfiability = SAT_WORD;
    }
  }

  util::printRow("s", satisfiability, keyWidth);
}

void Executor::printTypeRow(size_t keyWidth) {
  util::printRow("s type", projectedCounting ? "pmc" : (weightedCounting ? "wmc" : "mc"), keyWidth);
}

void Executor::printEstRow(const Number& solution, size_t keyWidth) {
  util::printRow("s log10-estimate", logCounting ? solution.fraction : solution.getLog10(), keyWidth);
}

void Executor::printArbRow(const Number& solution, bool frac, size_t keyWidth) {
  string key = "s exact arb ";

  if (weightedCounting) {
    if (frac) {
      util::printRow(key + "frac", solution, keyWidth);
    }
    else {
      util::printRow(key + "float", mpf_class(solution.quotient), keyWidth);
    }
  }
  else {
    util::printRow(key + "int", solution, keyWidth);
  }
}

void Executor::printDoubleRow(const Number& solution, size_t keyWidth) {
  Float f = solution.fraction;
  util::printRow("s exact double prec-sci", logCounting ? exp10l(f) : f, keyWidth);
}

Number Executor::printAdjustedSolutionRows(const Number& solution, bool unsatFlag, size_t keyWidth) {
  cout << DASH_LINE;
  Number adjustedSolution = getAdjustedSolution(solution);

  printSatRow(adjustedSolution, unsatFlag, keyWidth);
  printTypeRow(keyWidth);
  printEstRow(adjustedSolution, keyWidth);

  if (multiplePrecision) {
    printArbRow(adjustedSolution, false, keyWidth); // notation = weighted ? int : float
    if (weightedCounting) {
      printArbRow(adjustedSolution, true, keyWidth); // notation = frac
    }
  }
  else {
    printDoubleRow(adjustedSolution, keyWidth);
  }

  cout << DASH_LINE;
  return adjustedSolution;
}

string Executor::getShortModel(const Assignment& model, Int declaredVarCount) {
  string s;
  for (Int cnfVar = 1; cnfVar <= declaredVarCount; cnfVar++) {
    s += to_string(model.getValue(cnfVar));
  }
  return s;
}

string Executor::getLongModel(const Assignment& model, Int declaredVarCount) {
  string s;
  for (Int cnfVar = 1; cnfVar <= declaredVarCount; cnfVar++) {
    s += (model.getValue(cnfVar) ? " " : " -") + to_string(cnfVar);
  }
  return s;
}

void Executor::printShortMaximizer(const Assignment& maximizer, Int declaredVarCount) {
  cout << "v ";
  cout << getShortModel(maximizer, declaredVarCount);
  cout << "\n";
}

void Executor::printLongMaximizer(const Assignment& maximizer, Int declaredVarCount) {
  cout << "v";
  cout << getLongModel(maximizer, declaredVarCount);
  cout << "\n";
}

Assignment Executor::printMaximizerRows(const vector<Int>& ddVarToCnfVarMap, Int declaredVarCount) {
  vector<int> ddVarAssignment(ddVarToCnfVarMap.size(), -1); // uses init value -1 (neither 0 nor 1) to test assertion in function Cudd_Eval
  Assignment cnfVarAssignment;

  while (!maximizationStack.empty()) {
    pair<Int, Dd> ddVarAndDsgn = maximizationStack.back();
    Int ddVar = ddVarAndDsgn.first;
    Dd dsgn = ddVarAndDsgn.second;

    bool val = dsgn.evalAssignment(ddVarAssignment);
    ddVarAssignment[ddVar] = val;
    cnfVarAssignment.insert({ddVarToCnfVarMap.at(ddVar), val});

    maximizationStack.pop_back();
  }

  switch (maximizerFormat) {
    case NONE:
      break;
    case SHORT:
      printShortMaximizer(cnfVarAssignment, declaredVarCount);
      break;
    case LONG:
      printLongMaximizer(cnfVarAssignment, declaredVarCount);
      break;
    default:
      printShortMaximizer(cnfVarAssignment, declaredVarCount);
      printLongMaximizer(cnfVarAssignment, declaredVarCount);
  }

  return cnfVarAssignment;
}

Number Executor::verifyMaximizer(
  const JoinNonterminal* joinRoot,
  const Map<Int, Int>& cnfVarToDdVarMap,
  const vector<Int>& ddVarToCnfVarMap,
  const Assignment& maximizer
) {
  Dd dd = solveSubtree(
    joinRoot,
    cnfVarToDdVarMap,
    ddVarToCnfVarMap,
    Dd::newMgr(maxMem),
    maximizer
  );
  Number solution = dd.extractConst();
  return getAdjustedSolution(solution);
}

Executor::Executor(const JoinNonterminal* joinRoot, Int ddVarOrderHeuristic, Int sliceVarOrderHeuristic) {
  cout << "\n";
  cout << "c computing output...\n";

  TimePoint ddVarOrderStartPoint = util::getTimePoint();
  vector<Int> ddVarToCnfVarMap = joinRoot->getVarOrder(ddVarOrderHeuristic); // e.g. [42, 13], i.e. ddVarOrder
  if (verboseSolving >= 1) {
    util::printRow("diagramVarSeconds", util::getDuration(ddVarOrderStartPoint));
  }

  Map<Int, Int> cnfVarToDdVarMap; // e.g. {42: 0, 13: 1}
  for (Int ddVar = 0; ddVar < ddVarToCnfVarMap.size(); ddVar++) {
    Int cnfVar = ddVarToCnfVarMap.at(ddVar);
    cnfVarToDdVarMap[cnfVar] = ddVar;
  }

  setLogBound(joinRoot, cnfVarToDdVarMap, ddVarToCnfVarMap);

  Number solution = solveCnf(joinRoot, cnfVarToDdVarMap, ddVarToCnfVarMap, sliceVarOrderHeuristic);

  printVarDurations();
  printVarDdSizes();

  if (verboseSolving >= 1) {
    util::printRow("apparentSolution", solution);
  }

  if (logBound > -INF) {
    util::printRow("prunedDdCount", prunedDdCount);
    util::printRow("pruningSeconds", Dd::pruningDuration);
  }

  solution = printAdjustedSolutionRows(solution);

  if (maximizerFormat) {
    Assignment maximizer = printMaximizerRows(ddVarToCnfVarMap, joinRoot->cnf.declaredVarCount);
    if (maximizerVerification) {
      TimePoint maximizerVerificationStartPoint = util::getTimePoint();
      Number maximizerSolution = verifyMaximizer(
        joinRoot,
        cnfVarToDdVarMap,
        ddVarToCnfVarMap,
        maximizer
      );
      util::printRow("adjustedSolution", solution);
      util::printRow("maximizerSolution", maximizerSolution);
      util::printRow("solutionMatch", (solution - maximizerSolution).getAbsolute() < Number("1/1000000")); // 1e-6 is tolerance in ProCount paper
      if (verboseSolving >= 1) {
        util::printRow("maximizerVerificationSeconds", util::getDuration(maximizerVerificationStartPoint));
      }
    }
  }
}

/* class OptionDict ========================================================= */

string OptionDict::helpMaximizerFormat() {
  string s = "maximizer format" + util::useOption(EXIST_RANDOM_OPTION, "1") + ": ";
  for (auto it = MAXIMIZER_FORMATS.begin(); it != MAXIMIZER_FORMATS.end(); it++) {
    s += to_string(it->first) + "/" + it->second;
    if (next(it) != MAXIMIZER_FORMATS.end()) {
      s += ", ";
    }
  }
  return s + "; int";
}

string OptionDict::helpDdPackage() {
  string s = "diagram package: ";
  for (auto it = DD_PACKAGES.begin(); it != DD_PACKAGES.end(); it++) {
    s += it->first + "/" + it->second;
    if (next(it) != DD_PACKAGES.end()) {
      s += ", ";
    }
  }
  return s + "; string";
}

string OptionDict::helpJoinPriority() {
  string s = "join priority: ";
  for (auto it = JOIN_PRIORITIES.begin(); it != JOIN_PRIORITIES.end(); it++) {
    s += it->first + "/" + it->second;
    if (next(it) != JOIN_PRIORITIES.end()) {
      s += ", ";
    }
  }
  return s + "; string";
}

void OptionDict::runCommand() const {
  if (verboseSolving >= 1) {
    cout << "c processing command-line options...\n";
    util::printRow("cnfFile", cnfFilePath);
    util::printRow("weightedCounting", weightedCounting);
    util::printRow("projectedCounting", projectedCounting);
    util::printRow("existRandom", existRandom);
    util::printRow("diagramPackage", DD_PACKAGES.at(ddPackage));
    if (ddPackage == CUDD) {
      util::printRow("logCounting", logCounting);
    }
    if (!projectedCounting && existRandom && logCounting) {
      if (logBound > -INF) {
        util::printRow("logBound", logBound);
      }
      else if (!thresholdModel.empty()) {
        util::printRow("thresholdModel", thresholdModel);
      }
      else if (existPruning) {
        util::printRow("existPruning", existPruning);
      }
    }
    if (existRandom && ddPackage == CUDD) {
      util::printRow("maximizerFormat", MAXIMIZER_FORMATS.at(maximizerFormat));
    }
    if (maximizerFormat) {
      util::printRow("maximizerVerification", maximizerVerification);
    }
    if (!weightedCounting && maximizerFormat) {
      util::printRow("substitutionMaximization", substitutionMaximization);
    }
    util::printRow("plannerWaitSeconds", plannerWaitDuration);
    util::printRow("threadCount", threadCount);
    if (ddPackage == CUDD) {
      util::printRow("threadSliceCount", threadSliceCount);
    }
    util::printRow("randomSeed", randomSeed);
    util::printRow("diagramVarOrderHeuristic", (ddVarOrderHeuristic < 0 ? "INVERSE_" : "") + CNF_VAR_ORDER_HEURISTICS.at(abs(ddVarOrderHeuristic)));
    if (ddPackage == CUDD) {
      util::printRow("sliceVarOrderHeuristic", (sliceVarOrderHeuristic < 0 ? "INVERSE_" : "") + util::getVarOrderHeuristics().at(abs(sliceVarOrderHeuristic)));
      util::printRow("memSensitivityMegabytes", memSensitivity);
    }
    util::printRow("maxMemMegabytes", maxMem);
    if (ddPackage == SYLVAN) {
      util::printRow("tableRatio", tableRatio);
      util::printRow("initRatio", initRatio);
      util::printRow("multiplePrecision", multiplePrecision);
    }
    util::printRow("joinPriority", JOIN_PRIORITIES.at(joinPriority));
    cout << "\n";
  }

  try {
    JoinNode::cnf = Cnf(cnfFilePath);

    if (JoinNode::cnf.clauses.empty()) {
      cout << WARNING << "empty CNF\n";
      Executor::printAdjustedSolutionRows(logCounting ? Number() : Number("1"));
      return;
    }

    JoinTreeProcessor joinTreeProcessor(plannerWaitDuration);

    if (ddPackage == SYLVAN) { // initializes Sylvan
      lace_init(threadCount, 0);
      lace_startup(0, NULL, NULL);
      sylvan::sylvan_set_limits(maxMem * MEGA, tableRatio, initRatio);
      sylvan::sylvan_init_package();
      sylvan::sylvan_init_mtbdd();
      if (multiplePrecision) {
        sylvan::gmp_init();
      }
    }

    Executor executor(joinTreeProcessor.getJoinTreeRoot(), ddVarOrderHeuristic, sliceVarOrderHeuristic);

    if (ddPackage == SYLVAN) { // quits Sylvan
      sylvan::sylvan_quit();
      lace_exit();
    }
  }
  catch (UnsatException) {
    Executor::printAdjustedSolutionRows(logCounting ? Number(-INF) : Number(), true);
  }
}

OptionDict::OptionDict(int argc, char** argv) {
  cxxopts::Options options("dmc", "Diagram Model Counter (reads join tree from stdin)");
  options.set_width(105);
  options.add_options()
    (CNF_FILE_OPTION, "CNF file path; string (REQUIRED)", value<string>())
    (WEIGHTED_COUNTING_OPTION, "weighted counting: 0, 1; int", value<Int>()->default_value("0"))
    (PROJECTED_COUNTING_OPTION, "projected counting: 0, 1; int", value<Int>()->default_value("0"))
    (EXIST_RANDOM_OPTION, "existential-randomized stochastic satisfiability: 0, 1; int", value<Int>()->default_value("0"))
    (DD_PACKAGE_OPTION, helpDdPackage(), value<string>()->default_value(CUDD))
    (LOG_COUNTING_OPTION, "logarithmic counting" + util::useDdPackage(CUDD) + ": 0, 1; int", value<Int>()->default_value("0"))
    (LOG_BOUND_OPTION, "log10 of bound for existential pruning" + util::useOption(EXIST_RANDOM_OPTION, "1") + "; float", value<string>()->default_value(to_string(-INF))) // cxxopts fails to parse "-inf" as Float
    (THRESHOLD_MODEL_OPTION, "threshold model for existential pruning" + util::useOption(EXIST_RANDOM_OPTION, "1") + "; string", value<string>()->default_value(""))
    (EXIST_PRUNING_OPTION, "existential pruning using CryptoMiniSat" + util::useOption(EXIST_RANDOM_OPTION, "1") + ": 0, 1; int", value<Int>()->default_value("0"))
    (MAXIMIZER_FORMAT_OPTION, helpMaximizerFormat(), value<Int>()->default_value(to_string(NONE)))
    (MAXIMIZER_VERIFICATION_OPTION, "maximizer verification" + util::useOption(MAXIMIZER_FORMAT_OPTION, to_string(NONE), ">") + ": 0, 1; int", value<Int>()->default_value("0"))
    (SUBSTITUTION_MAXIMIZATION_OPTION, "substitution-based maximization" + util::useOption(MAXIMIZER_FORMAT_OPTION, to_string(NONE), ">") + ": 0, 1; int", value<Int>()->default_value("0"))
    (PLANNER_WAIT_OPTION, "planner wait duration (in seconds); float", value<Float>()->default_value("0.0"))
    (THREAD_COUNT_OPTION, "thread count, or 0 for hardware_concurrency value; int", value<Int>()->default_value("1"))
    (THREAD_SLICE_COUNT_OPTION, "thread slice count" + util::useDdPackage(CUDD) + "; int", value<Int>()->default_value("1"))
    (RANDOM_SEED_OPTION, "random seed; int", value<Int>()->default_value("0"))
    (DD_VAR_OPTION, util::helpVarOrderHeuristic("diagram"), value<Int>()->default_value(to_string(MCS)))
    (SLICE_VAR_OPTION, util::helpVarOrderHeuristic("slice"), value<Int>()->default_value(to_string(BIGGEST_NODE)))
    (MEM_SENSITIVITY_OPTION, "mem sensitivity (in MB) for reporting usage" + util::useDdPackage(CUDD) + "; float", value<Float>()->default_value("1e3"))
    (MAX_MEM_OPTION, "max mem (in MB) for unique table and cache table combined; float", value<Float>()->default_value("4e3"))
    (TABLE_RATIO_OPTION, "table ratio" + util::useDdPackage(SYLVAN) + ": log2(unique_size/cache_size); int", value<Int>()->default_value("1"))
    (INIT_RATIO_OPTION, "init ratio for tables" + util::useDdPackage(SYLVAN) + ": log2(max_size/init_size); int", value<Int>()->default_value("10"))
    (MULTIPLE_PRECISION_OPTION, "multiple precision" + util::useDdPackage(SYLVAN) + ": 0, 1; int", value<Int>()->default_value("0"))
    (JOIN_PRIORITY_OPTION, helpJoinPriority(), value<string>()->default_value(SMALLEST_PAIR))
    (VERBOSE_CNF_OPTION, "verbose CNF processing: " + INPUT_VERBOSITY_LEVELS, value<Int>()->default_value("0"))
    (VERBOSE_JOIN_TREE_OPTION, "verbose join-tree processing: " + INPUT_VERBOSITY_LEVELS, value<Int>()->default_value("0"))
    (VERBOSE_PROFILING_OPTION, "verbose profiling: 0, 1, 2; int", value<Int>()->default_value("0"))
    (VERBOSE_SOLVING_OPTION, util::helpVerboseSolving(), value<Int>()->default_value("1"))
  ;
  cxxopts::ParseResult result = options.parse(argc, argv);
  if (result.count(CNF_FILE_OPTION)) {
    cnfFilePath = result[CNF_FILE_OPTION].as<string>();

    weightedCounting = result[WEIGHTED_COUNTING_OPTION].as<Int>(); // global var

    projectedCounting = result[PROJECTED_COUNTING_OPTION].as<Int>(); // global var

    existRandom = result[EXIST_RANDOM_OPTION].as<Int>(); // global var

    ddPackage = result[DD_PACKAGE_OPTION].as<string>(); // global var
    assert(DD_PACKAGES.contains(ddPackage));

    logCounting = result[LOG_COUNTING_OPTION].as<Int>(); // global var
    assert(!logCounting || ddPackage == CUDD);

    logBound = stold(result[LOG_BOUND_OPTION].as<string>()); // global var
    assert(logBound == -INF || !projectedCounting);
    assert(logBound == -INF || existRandom);
    assert(logBound == -INF || logCounting);

    thresholdModel = result[THRESHOLD_MODEL_OPTION].as<string>(); // global var
    assert(thresholdModel.empty() || !projectedCounting);
    assert(thresholdModel.empty() || existRandom);
    assert(thresholdModel.empty() || logCounting);
    assert(thresholdModel.empty() || logBound == -INF);

    existPruning = result[EXIST_PRUNING_OPTION].as<Int>(); // global var
    assert(!existPruning || !projectedCounting);
    assert(!existPruning || existRandom);
    assert(!existPruning || logCounting);
    assert(!existPruning || logBound == -INF);
    assert(!existPruning || thresholdModel.empty());

    maximizerFormat = result[MAXIMIZER_FORMAT_OPTION].as<Int>(); // global var
    assert(MAXIMIZER_FORMATS.contains(maximizerFormat));
    assert(!maximizerFormat || existRandom);
    assert(!maximizerFormat || ddPackage == CUDD);

    maximizerVerification = result[MAXIMIZER_VERIFICATION_OPTION].as<Int>(); // global var
    assert(!maximizerVerification || maximizerFormat);

    substitutionMaximization = result[SUBSTITUTION_MAXIMIZATION_OPTION].as<Int>(); // global var
    assert(!substitutionMaximization || !weightedCounting);
    assert(!substitutionMaximization || maximizerFormat);

    plannerWaitDuration = result[PLANNER_WAIT_OPTION].as<Float>();
    plannerWaitDuration = max(plannerWaitDuration, 0.0l);

    threadCount = result[THREAD_COUNT_OPTION].as<Int>(); // global var
    if (threadCount <= 0) {
      threadCount = thread::hardware_concurrency();
    }
    assert(threadCount > 0);

    threadSliceCount = result[THREAD_SLICE_COUNT_OPTION].as<Int>(); // global var
    threadSliceCount = max(threadSliceCount, 1ll);
    assert(threadSliceCount == 1 || ddPackage == CUDD);

    randomSeed = result[RANDOM_SEED_OPTION].as<Int>(); // global var

    ddVarOrderHeuristic = result[DD_VAR_OPTION].as<Int>();
    assert(CNF_VAR_ORDER_HEURISTICS.contains(abs(ddVarOrderHeuristic)));

    sliceVarOrderHeuristic = result[SLICE_VAR_OPTION].as<Int>();
    assert(util::getVarOrderHeuristics().contains(abs(sliceVarOrderHeuristic)));

    memSensitivity = result[MEM_SENSITIVITY_OPTION].as<Float>(); // global var

    maxMem = result[MAX_MEM_OPTION].as<Float>(); // global var

    tableRatio = result[TABLE_RATIO_OPTION].as<Int>();

    initRatio = result[INIT_RATIO_OPTION].as<Int>();

    multiplePrecision = result[MULTIPLE_PRECISION_OPTION].as<Int>(); // global var
    assert(!multiplePrecision || ddPackage == SYLVAN);

    joinPriority = result[JOIN_PRIORITY_OPTION].as<string>(); //global var
    assert(JOIN_PRIORITIES.contains(joinPriority));

    verboseCnf = result[VERBOSE_CNF_OPTION].as<Int>(); // global var

    verboseJoinTree = result[VERBOSE_JOIN_TREE_OPTION].as<Int>(); // global var

    verboseProfiling = result[VERBOSE_PROFILING_OPTION].as<Int>(); // global var
    assert(verboseProfiling <= 0 || threadCount == 1);

    verboseSolving = result[VERBOSE_SOLVING_OPTION].as<Int>(); // global var

    toolStartPoint = util::getTimePoint(); // global var
    runCommand();
    util::printRow("seconds", util::getDuration(toolStartPoint));
  }
  else {
    cout << options.help();
  }
}

/* global functions ========================================================= */

int main(int argc, char** argv) {
  cout << std::unitbuf; // enables automatic flushing
  OptionDict(argc, argv);
}
