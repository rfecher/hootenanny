/*
 * This file is part of Hootenanny.
 *
 * Hootenanny is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --------------------------------------------------------------------
 *
 * The following copyright notices are generated automatically. If you
 * have a new notice to add, please use the format:
 * " * @copyright Copyright ..."
 * This will properly maintain the copyright information. Maxar
 * copyrights will be updated automatically.
 *
 * @copyright Copyright (C) 2014, 2015, 2017, 2018, 2019, 2020, 2021 Maxar (http://www.maxar.com/)
 */

// Hoot
#include <hoot/core/TestUtils.h>
#include <hoot/core/algorithms/perty/PertyMatchScorer.h>
#include <hoot/core/algorithms/perty/PertyTestRunner.h>
#include <hoot/core/algorithms/perty/PertyTestRunResult.h>
#include <hoot/core/io/OsmXmlReader.h>
#include <hoot/core/util/ConfigOptions.h>
#include <hoot/core/util/FileUtils.h>
#include <hoot/core/util/Settings.h>

namespace hoot
{

class PertyTestRunnerTest : public HootTestFixture
{
  CPPUNIT_TEST_SUITE(PertyTestRunnerTest);
  CPPUNIT_TEST(runDynamicVariablesTest);
  CPPUNIT_TEST(runNoDynamicVariablesTest);
  CPPUNIT_TEST(runHigherScoresOutsideOfVarianceAllowedToPassTest);
  CPPUNIT_TEST(runInvalidNumTestRunsTest);
  CPPUNIT_TEST(runInvalidNumTestSimulationsTest);
  CPPUNIT_TEST(runInvalidNumExpectedScoresTest);
  CPPUNIT_TEST(runInvalidAllowedScoreVarianceTest);
  CPPUNIT_TEST(runBadDynamicVariableTest);
  CPPUNIT_TEST(runInvalidExpectedScoreTest);
  CPPUNIT_TEST_SUITE_END();

public:

  PertyTestRunnerTest()
    : HootTestFixture("test-files/algorithms/perty/PertyTestRunnerTest/",
                      "test-output/algorithms/perty/PertyTestRunnerTest/")
  {
    setResetType(ResetConfigs);
    FileUtils::makeDir(_outputPath + "Dynamic");
    FileUtils::makeDir(_outputPath + "Static");
    FileUtils::makeDir(_outputPath + "Variance");
  }

  void runDynamicVariablesTest()
  {
    QFile resultsFile(_outputPath + "Dynamic/results");
    resultsFile.remove();
    QFile resultsPlotFile(_outputPath + "Dynamic/results-plot.dat");
    resultsPlotFile.remove();

    Settings testSettings = conf();
    testSettings.set(ConfigOptions::getPertySearchDistanceKey(), QString::number(15.0));

    PertyTestRunner testRunner;
    testRunner.setConfiguration(testSettings);
    const int numTestRuns = 2;
    testRunner.setAllowedScoreVariance(0.15);
    testRunner.setDynamicVariableIncrement(0.1);
    QStringList dynamicVariables;
    dynamicVariables.append("perty.systematic.error.x");
    dynamicVariables.append("perty.systematic.error.y");
    testRunner.setDynamicVariables(dynamicVariables);
    testRunner.setDynamicVariableStartValue(1.0);
    QStringList expectedScores;
    for (int i = 0; i < numTestRuns; i++)
    {
      expectedScores.append("1.0");
    }
    testRunner.setExpectedScores(expectedScores);
    testRunner.setNumTestRuns(numTestRuns);
    testRunner.setNumTestSimulations(2);
    testRunner.setGenerateMapStats(true);
    testRunner.setFailOnBetterScore(true);

    testRunner._returnTestScores = true;
    testRunner._testScores.append(0.9);
    testRunner._testScores.append(0.8);
    testRunner._testScores.append(0.75);
    testRunner._testScores.append(0.85);

    const QList<std::shared_ptr<const PertyTestRunResult>> results =
      testRunner.runTest(
        _inputPath + "PertyTestRunnerTest-reference-in-1.osm",
        _outputPath + "Dynamic");
    QString testRunResultsStr = "";
    QString testRunPlotResultsStr = "";
    double dynamicVariableValue = testRunner._dynamicVariableStartValue;
    for (QList<std::shared_ptr<const PertyTestRunResult>>::const_iterator it = results.begin();
         it != results.end(); ++it)
    {
      std::shared_ptr<const PertyTestRunResult> result = *it;
      testRunResultsStr += result->toString();
      testRunPlotResultsStr +=
        QString::number(dynamicVariableValue) + " " + QString::number(result->getScore()) + "\n";
      dynamicVariableValue += testRunner._dynamicVariableIncrement;
    }

    CPPUNIT_ASSERT(resultsFile.exists());
    if (!resultsFile.open(QFile::ReadOnly | QFile::Text))
    {
      CPPUNIT_FAIL("Unable to open results file.");
    }
    QTextStream in(&resultsFile);
    CPPUNIT_ASSERT(in.readAll() == testRunResultsStr);
    resultsFile.close();

    CPPUNIT_ASSERT(resultsPlotFile.exists());
    if (!resultsPlotFile.open(QFile::ReadOnly | QFile::Text))
    {
      CPPUNIT_FAIL("Unable to open results plot file.");
    }
    QTextStream in2(&resultsPlotFile);
    CPPUNIT_ASSERT(in2.readAll() == testRunPlotResultsStr);
    resultsPlotFile.close();

    CPPUNIT_ASSERT("1.1" == testRunner._settings.get("perty.systematic.error.x").toString());
    CPPUNIT_ASSERT("1.1" == testRunner._settings.get("perty.systematic.error.y").toString());

    CPPUNIT_ASSERT_EQUAL(2, results.size());

    std::shared_ptr<const PertyTestRunResult> result = results[0];
    CPPUNIT_ASSERT_EQUAL(1, result->getTestRunNum());
    CPPUNIT_ASSERT(result->testPassed());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.85, result->getScore(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.15, result->getAllowedScoreVariance(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.1, result->getDynamicVariableIncrement(), 0.00001);
    CPPUNIT_ASSERT_EQUAL(2, result->getDynamicVariables().size());
    HOOT_STR_EQUALS("perty.systematic.error.x", result->getDynamicVariables()[0]);
    HOOT_STR_EQUALS("perty.systematic.error.y", result->getDynamicVariables()[1]);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, result->getDynamicVariableStartingValue(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, result->getDynamicVariableValue(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, result->getExpectedScore(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.15, result->getScoreVariance(), 0.00001);

    result = results[1];
    CPPUNIT_ASSERT_EQUAL(2, result->getTestRunNum());
    CPPUNIT_ASSERT(!result->testPassed());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.8, result->getScore(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.15, result->getAllowedScoreVariance(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.1, result->getDynamicVariableIncrement(), 0.00001);
    CPPUNIT_ASSERT_EQUAL(2, result->getDynamicVariables().size());
    HOOT_STR_EQUALS("perty.systematic.error.x", result->getDynamicVariables()[0]);
    HOOT_STR_EQUALS("perty.systematic.error.y", result->getDynamicVariables()[1]);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, result->getDynamicVariableStartingValue(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.1, result->getDynamicVariableValue(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, result->getExpectedScore(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.2, result->getScoreVariance(), 0.00001);
  }

  void runNoDynamicVariablesTest()
  {
    QFile resultsFile(_outputPath + "Static/results");
    resultsFile.remove();
    QFile resultsPlotFile(_outputPath + "Static/results-plot.dat");
    resultsPlotFile.remove();

    Settings testSettings = conf();
    testSettings.set(ConfigOptions::getPertySearchDistanceKey(), QString::number(15.0));

    PertyTestRunner testRunner;
    testRunner.setConfiguration(testSettings);
    const int numTestRuns = 2;
    testRunner.setAllowedScoreVariance(0.15);
    testRunner.setDynamicVariableIncrement(0.0);
    testRunner.setDynamicVariables(QStringList());
    testRunner.setDynamicVariableStartValue(0.0);
    QStringList expectedScores;
    for (int i = 0; i < numTestRuns; i++)
    {
      expectedScores.append("1.0");
    }
    testRunner.setExpectedScores(expectedScores);
    testRunner.setNumTestRuns(numTestRuns);
    testRunner.setNumTestSimulations(2);
    testRunner.setGenerateMapStats(true);
    testRunner.setFailOnBetterScore(true);

    testRunner._returnTestScores = true;
    testRunner._testScores.append(0.9);
    testRunner._testScores.append(0.8);
    testRunner._testScores.append(0.75);
    testRunner._testScores.append(0.85);

    const QList<std::shared_ptr<const PertyTestRunResult>> results =
      testRunner.runTest(
        _inputPath + "PertyTestRunnerTest-reference-in-1.osm",
        _outputPath + "Static");
    QString testRunResultsStr = "";
    for (QList<std::shared_ptr<const PertyTestRunResult>>::const_iterator it = results.begin();
         it != results.end(); ++it)
    {
      std::shared_ptr<const PertyTestRunResult> result = *it;
      testRunResultsStr += result->toString();
    }

    CPPUNIT_ASSERT(resultsFile.exists());
    if (!resultsFile.open(QFile::ReadOnly | QFile::Text))
    {
      CPPUNIT_FAIL("Unable to open results file.");
    }
    QTextStream in(&resultsFile);
    CPPUNIT_ASSERT(in.readAll() == testRunResultsStr);
    resultsFile.close();

    //plot file only written when dynamic vars are used
    CPPUNIT_ASSERT(!resultsPlotFile.exists());

    CPPUNIT_ASSERT_EQUAL(2, results.size());

    std::shared_ptr<const PertyTestRunResult> result = results[0];
    CPPUNIT_ASSERT_EQUAL(1, result->getTestRunNum());
    CPPUNIT_ASSERT(result->testPassed());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.85, result->getScore(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.15, result->getAllowedScoreVariance(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, result->getDynamicVariableIncrement(), 0.00001);
    CPPUNIT_ASSERT_EQUAL(0, result->getDynamicVariables().size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, result->getDynamicVariableStartingValue(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, result->getDynamicVariableValue(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, result->getExpectedScore(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.15, result->getScoreVariance(), 0.00001);
    CPPUNIT_ASSERT_EQUAL(2, result->getSimulationScores().size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.9, result->getSimulationScores().at(0), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.8, result->getSimulationScores().at(1), 0.00001);

    result = results[1];
    CPPUNIT_ASSERT_EQUAL(2, result->getTestRunNum());
    CPPUNIT_ASSERT(!result->testPassed());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.8, result->getScore(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.15, result->getAllowedScoreVariance(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, result->getDynamicVariableIncrement(), 0.00001);
    CPPUNIT_ASSERT_EQUAL(0, result->getDynamicVariables().size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, result->getDynamicVariableStartingValue(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, result->getDynamicVariableValue(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, result->getExpectedScore(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.2, result->getScoreVariance(), 0.00001);
    CPPUNIT_ASSERT_EQUAL(2, result->getSimulationScores().size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.75, result->getSimulationScores().at(0), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.85, result->getSimulationScores().at(1), 0.00001);
  }

  void runHigherScoresOutsideOfVarianceAllowedToPassTest()
  {
    QFile resultsFile(_outputPath + "Variance/results");
    resultsFile.remove();
    QFile resultsPlotFile(_outputPath + "Variance/results-plot.dat");
    resultsPlotFile.remove();

    Settings testSettings = conf();
    testSettings.set(ConfigOptions::getPertySearchDistanceKey(), QString::number(15.0));

    PertyTestRunner testRunner;
    testRunner.setConfiguration(testSettings);
    const int numTestRuns = 2;
    testRunner.setAllowedScoreVariance(0.15);
    testRunner.setDynamicVariableIncrement(0.0);
    testRunner.setDynamicVariables(QStringList());
    testRunner.setDynamicVariableStartValue(0.0);
    QStringList expectedScores;
    expectedScores.append("0.69");
    expectedScores.append("0.75");
    testRunner.setExpectedScores(expectedScores);
    testRunner.setNumTestRuns(numTestRuns);
    testRunner.setNumTestSimulations(2);
    testRunner.setGenerateMapStats(true);
    testRunner.setFailOnBetterScore(false);

    testRunner._returnTestScores = true;
    testRunner._testScores.append(0.9);
    testRunner._testScores.append(0.8);
    testRunner._testScores.append(0.75);
    testRunner._testScores.append(0.85);

    const QList<std::shared_ptr<const PertyTestRunResult>> results =
      testRunner.runTest(
        _inputPath + "PertyTestRunnerTest-reference-in-1.osm",
        _outputPath + "Variance");
    QString testRunResultsStr = "";
    QString testRunPlotResultsStr = "";
    double dynamicVariableValue = testRunner._dynamicVariableStartValue;
    for (QList<std::shared_ptr<const PertyTestRunResult>>::const_iterator it = results.begin();
         it != results.end(); ++it)
    {
      std::shared_ptr<const PertyTestRunResult> result = *it;
      testRunResultsStr += result->toString();
      testRunPlotResultsStr +=
        QString::number(dynamicVariableValue) + " " + QString::number(result->getScore()) + "\n";
      dynamicVariableValue += testRunner._dynamicVariableIncrement;
    }

    CPPUNIT_ASSERT(resultsFile.exists());
    if (!resultsFile.open(QFile::ReadOnly | QFile::Text))
    {
      CPPUNIT_FAIL("Unable to open results file.");
    }
    QTextStream in(&resultsFile);
    CPPUNIT_ASSERT(in.readAll() == testRunResultsStr);
    resultsFile.close();

    //plot file only written when dynamic vars are used
    CPPUNIT_ASSERT(!resultsPlotFile.exists());

    CPPUNIT_ASSERT_EQUAL(2, results.size());

    std::shared_ptr<const PertyTestRunResult> result = results[0];
    CPPUNIT_ASSERT_EQUAL(1, result->getTestRunNum());
    CPPUNIT_ASSERT(result->testPassed());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.85, result->getScore(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.15, result->getAllowedScoreVariance(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, result->getDynamicVariableIncrement(), 0.00001);
    CPPUNIT_ASSERT_EQUAL(0, result->getDynamicVariables().size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, result->getDynamicVariableStartingValue(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, result->getDynamicVariableValue(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.69, result->getExpectedScore(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.16, result->getScoreVariance(), 0.00001);
    CPPUNIT_ASSERT_EQUAL(2, result->getSimulationScores().size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.9, result->getSimulationScores().at(0), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.8, result->getSimulationScores().at(1), 0.00001);

    result = results[1];
    CPPUNIT_ASSERT_EQUAL(2, result->getTestRunNum());
    CPPUNIT_ASSERT(result->testPassed());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.8, result->getScore(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.15, result->getAllowedScoreVariance(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, result->getDynamicVariableIncrement(), 0.00001);
    CPPUNIT_ASSERT_EQUAL(0, result->getDynamicVariables().size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, result->getDynamicVariableStartingValue(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, result->getDynamicVariableValue(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.75, result->getExpectedScore(), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.05, result->getScoreVariance(), 0.00001);
    CPPUNIT_ASSERT_EQUAL(2, result->getSimulationScores().size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.75, result->getSimulationScores().at(0), 0.00001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.85, result->getSimulationScores().at(1), 0.00001);
  }

  void runInvalidNumTestRunsTest()
  {
    QString exceptionMsg;
    try
    {
      PertyTestRunner().setNumTestRuns(0);
    }
    catch (const HootException& e)
    {
      exceptionMsg = e.what();
    }

    CPPUNIT_ASSERT(
      QString::fromStdString(exceptionMsg.toStdString()).contains("Invalid number of test runs"));

    PertyTestRunner testRunner;
    QStringList expectedScores;
    expectedScores.append("1.0");
    expectedScores.append("1.0");
    testRunner.setExpectedScores(expectedScores);
    testRunner.setNumTestRuns(1);
    try
    {
      testRunner.runTest("", "");
    }
    catch (const HootException& e)
    {
      exceptionMsg = e.what();
    }

    CPPUNIT_ASSERT(
      QString::fromStdString(exceptionMsg.toStdString()).contains("does not equal the number of expected scores"));
  }

  void runInvalidNumTestSimulationsTest()
  {
    QString exceptionMsg;
    try
    {
      PertyTestRunner().setNumTestSimulations(0);
    }
    catch (const HootException& e)
    {
      exceptionMsg = e.what();
    }

    CPPUNIT_ASSERT(
      QString::fromStdString(exceptionMsg.toStdString()).contains("Invalid number of test simulations"));
  }

  void runInvalidNumExpectedScoresTest()
  {
    PertyTestRunner testRunner;
    testRunner.setNumTestRuns(1);
    QStringList expectedScores;
    expectedScores.append("1.0");
    expectedScores.append("1.0");
    testRunner.setExpectedScores(expectedScores);
    QString exceptionMsg;
    try
    {
      testRunner.runTest("", "");
    }
    catch (const HootException& e)
    {
      exceptionMsg = e.what();
    }

    CPPUNIT_ASSERT(
      QString::fromStdString(exceptionMsg.toStdString()).contains("does not equal the number of expected scores"));
  }

  void runInvalidAllowedScoreVarianceTest()
  {
    QString exceptionMsg;
    try
    {
      PertyTestRunner().setAllowedScoreVariance(-0.1);
    }
    catch (const HootException& e)
    {
      exceptionMsg = e.what();
    }

    CPPUNIT_ASSERT(
      QString::fromStdString(exceptionMsg.toStdString()).contains("Invalid allowed score variance"));

    try
    {
      PertyTestRunner().setAllowedScoreVariance(1.1);
    }
    catch (const HootException& e)
    {
      exceptionMsg = e.what();
    }

    CPPUNIT_ASSERT(
      QString::fromStdString(exceptionMsg.toStdString()).contains("Invalid allowed score variance"));
  }

  void runBadDynamicVariableTest()
  {
    PertyTestRunner testRunner;
    QStringList dynamicVariables;
    dynamicVariables.append("perty.systematic.error.x");
    dynamicVariables.append("notperty.systematic.error.y");
    QString exceptionMsg;
    try
    {
      testRunner.setDynamicVariables(dynamicVariables);
    }
    catch (const HootException& e)
    {
      exceptionMsg = e.what();
    }

    CPPUNIT_ASSERT(
      QString::fromStdString(exceptionMsg.toStdString()).contains(
        "Only PERTY variables may be manipulated during a PERTY test"));
  }

  void runInvalidExpectedScoreTest()
  {
    PertyTestRunner testRunner;
    QStringList expectedScores;
    expectedScores.append("blah");
    QString exceptionMsg;
    try
    {
      testRunner.setExpectedScores(expectedScores);
    }
    catch (const HootException& e)
    {
      exceptionMsg = e.what();
    }

    CPPUNIT_ASSERT(
      QString::fromStdString(exceptionMsg.toStdString()).contains("Error parsing expected score value"));
  }

};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(PertyTestRunnerTest, "slow");

}
