#include "formula.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <regex>
#include <sstream>

#include "FormulaAST.h"
#include "log/easylogging++.h"

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError formula_error) {
  return output << formula_error.ToString();
}

namespace {
class Formula : public FormulaInterface {
 public:
  explicit Formula(std::string expression) try
      : formula_ast_(ParseFormulaAST(expression)) {
  } catch (...) {
    throw FormulaException("Failed to parse formula"s);
  }

  Value Evaluate(const SheetInterface& sheet) const {
    try {
      LOG(DEBUG) << "Evaluating formula: " << GetExpression();
      std::function<double(Position)> func =
          [&sheet](const Position position) -> double {
        if (!position.IsValid()) {
          throw FormulaError(FormulaError::Category::Ref);
        }

        const auto* cell = sheet.GetCellInterface(position);
        if (!cell) {
          return 0.0;
        }

        const auto& value = cell->GetValue();
        if (std::holds_alternative<double>(value)) {
          LOG(DEBUG) << "Cell " << position.ToString() << " is double";
          return std::get<double>(value);
        }

        if (std::holds_alternative<std::string>(value)) {
          LOG(DEBUG) << "Cell " << position.ToString() << " is string";
          const auto& string_value = std::get<std::string>(value);
          std::regex regex_double(R"(^\s*([-+]?\d+(?:\.\d+)?)\s*$)");
          std::smatch match;
          if (std::regex_match(string_value, match, regex_double)) {
            LOG(DEBUG) << "Cell " << position.ToString() << " is double";
            return std::stod(match[1]);
          }

          throw FormulaError(FormulaError::Category::Value);
        }

        throw FormulaError(std::get<FormulaError>(value));
      };

      return formula_ast_.Execute(func);
    } catch (const FormulaError& formula_error) {
      return formula_error;
    }
  }

  std::string GetExpression() const override {
    std::ostringstream out;
    formula_ast_.PrintFormula(out);
    return out.str();
  }

  std::vector<Position> GetReferencedCells() const override {
    LOG(DEBUG) << "Get referenced cells";
    std::vector<Position> cell_positions;
    for (const auto& cell : formula_ast_.GetCells()) {
      if (cell.IsValid()) {
        cell_positions.push_back(cell);
      }
    }
    std::sort(cell_positions.begin(), cell_positions.end());
    cell_positions.erase(
        std::unique(cell_positions.begin(), cell_positions.end()),
        cell_positions.end());
    LOG(DEBUG) << "Referenced cells: " << cell_positions.size();
    return cell_positions;
  }

 private:
  FormulaAST formula_ast_;
};

}  // end namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
  return std::make_unique<Formula>(std::move(expression));
}