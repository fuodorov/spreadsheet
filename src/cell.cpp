#include "cell.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <set>
#include <stack>
#include <string>

#include "sheet.h"
#include "log/easylogging++.h"

Cell::Cell(Sheet& sheet)
    : impl_(std::make_unique<EmptyImpl>()), sheet_(sheet) {}

Cell::~Cell() = default;

void Cell::Set(std::string content, Position position, Sheet* sheet) {
  LOG(DEBUG) << "Set cell " << position.ToString() << " to " << content;

  std::unique_ptr<Impl> impl;

  if (content.empty()) {
    LOG(DEBUG) << "Empty cell";
    impl = std::make_unique<EmptyImpl>();
  }

  if (content.size() >= 2 && content[0] == FORMULA_SIGN) {
    LOG(DEBUG) << "Formula cell";
    impl = std::make_unique<FormulaImpl>(std::move(content), sheet_);

    for (Position cell : impl->GetReferencedCells()) {
      if (cell.IsValid() && !sheet->GetCellInterface(cell)) {
        sheet->SetCell(cell, "");
      }
    }
  } else {
    impl = std::make_unique<TextImpl>(std::move(content));
  }

  if (FindLoop(*impl, position)) {
    throw CircularDependencyException("Circular dependency");
  }

  impl_ = std::move(impl);

  for (Cell* cell : calc_cells_) {
    cell->use_cells_.erase(this);
  }

  use_cells_.clear();

  for (const auto& referenced_cell : impl_->GetReferencedCells()) {
    Cell* cell = sheet_.GetCell(referenced_cell);

    if (!cell) {
      sheet_.SetCell(referenced_cell, "");
      cell = sheet_.GetCell(referenced_cell);
    }

    use_cells_.insert(cell);
    cell->calc_cells_.insert(this);
  }

  ClearCache();
}

bool Cell::IsLoop(Cell* cell, std::unordered_set<Cell*>& cells,
                  const Position position) {
  for (auto cell : cell->GetReferencedCells()) {
    if (position == cell) {
      return true;
    }

    Cell* referenced_cell = sheet_.GetCell(cell);
    if (cells.find(referenced_cell) == cells.end()) {
      cells.insert(referenced_cell);
      if (IsLoop(referenced_cell, cells, position)) return true;
    }
  }

  return false;
}

bool Cell::FindLoop(const Impl& impl, Position position) {
  LOG(DEBUG) << "Find loop for " << position.ToString();
  const Position position_const = position;
  std::unordered_set<Cell*> cells;

  for (const auto& cell : impl.GetReferencedCells()) {
    LOG(DEBUG) << "Cell " << cell.ToString();
    if (cell == position) {
      return true;
    }

    Cell* referenced_cell = sheet_.GetCell(cell);
    cells.insert(referenced_cell);
    if (IsLoop(referenced_cell, cells, position_const)) {
      LOG(DEBUG) << "Loop found";
      return true;
    }
  }

  return false;
}

void Cell::Clear() { Set("", pos_, &sheet_); }

Cell::Value Cell::GetValue() const { return impl_->GetValue(); }

std::string Cell::GetText() const { return impl_->GetText(); }

std::vector<Position> Cell::GetReferencedCells() const {
  return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const { return !calc_cells_.empty(); }

void Cell::ClearCache() {
  if (!impl_->IsEmptyCache()) {
    impl_->ClearCache();

    for (Cell* cell : calc_cells_) {
      cell->ClearCache();
    }
  }
}

std::vector<Position> Cell::Impl::GetReferencedCells() const { return {}; }

bool Cell::Impl::IsEmptyCache() { return false; }

void Cell::Impl::ClearCache() {}

Cell::Value Cell::EmptyImpl::GetValue() const { return ""; }

std::string Cell::EmptyImpl::GetText() const { return ""; }

Cell::TextImpl::TextImpl(std::string content) : text_(std::move(content)) {}

Cell::Value Cell::TextImpl::GetValue() const {
  if (text_.empty()) {
    throw FormulaException("Empty cell");
  }

  return text_.at(0) == ESCAPE_SIGN ? text_.substr(1) : text_;
}

std::string Cell::TextImpl::GetText() const { return text_; }

Cell::FormulaImpl::FormulaImpl(std::string content, SheetInterface& sheet)
    : formula_(ParseFormula(content.substr(1))), sheet_(sheet) {}

Cell::Value Cell::FormulaImpl::GetValue() const {
  LOG(DEBUG) << "Get value for formula " << formula_->GetExpression();
  if (!db_) {
    LOG(DEBUG) << "Evaluate formula";
    db_ = formula_->Evaluate(sheet_);
  }
  return std::visit([](auto& helper) { return Value(helper); }, *db_);
}

std::string Cell::FormulaImpl::GetText() const {
  return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
  return formula_->GetReferencedCells();
}

bool Cell::FormulaImpl::IsEmptyCache() { return !db_.has_value(); }

void Cell::FormulaImpl::ClearCache() { db_.reset(); }
