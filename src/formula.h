#pragma once

#include <memory>
#include <vector>

#include "FormulaAST.h"
#include "common.h"

class FormulaInterface {
 public:
  using Value = std::variant<double, FormulaError>;

  virtual ~FormulaInterface() = default;
  virtual Value Evaluate(const SheetInterface& sheet) const = 0;

  virtual std::string GetExpression() const = 0;
  virtual std::vector<Position> GetReferencedCells() const = 0;
};

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression);
