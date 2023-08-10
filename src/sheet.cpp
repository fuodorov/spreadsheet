#include "sheet.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

#include "cell.h"
#include "common.h"

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position position, std::string text) {
  if (!position.IsValid()) {
    throw InvalidPositionException("Position is not valid.");
  }

  if (position.row >= int(std::size(spreadsheet_))) {
    spreadsheet_.resize(position.row + 1);
  }

  if (position.col >= int(std::size(spreadsheet_[position.row]))) {
    spreadsheet_[position.row].resize(position.col + 1);
  }

  if (!spreadsheet_[position.row][position.col]) {
    spreadsheet_[position.row][position.col] = std::make_unique<Cell>(*this);
  }

  spreadsheet_[position.row][position.col]->Set(std::move(text), position,
                                                this);
}

CellInterface* Sheet::GetCellInterface(Position position) {
  if (!position.IsValid()) {
    throw InvalidPositionException("Position is not valid.");
  }

  return position.row < int(std::size(spreadsheet_)) &&
                 position.col < int(std::size(spreadsheet_[position.row]))
             ? spreadsheet_[position.row][position.col].get()
             : nullptr;
}

const CellInterface* Sheet::GetCellInterface(Position position) const {
  if (!position.IsValid()) {
    throw InvalidPositionException("Position is not valid.");
  }

  if (position.row < int(std::size(spreadsheet_)) &&
      position.col < int(std::size(spreadsheet_[position.row]))) {
    return spreadsheet_[position.row][position.col].get()->GetText() == ""
               ? nullptr
               : spreadsheet_[position.row][position.col].get();
  } else {
    return nullptr;
  }
}

Cell* Sheet::GetCell(Position position) {
  if (!position.IsValid()) {
    throw InvalidPositionException("Position is not valid.");
  }

  return position.row < int(std::size(spreadsheet_)) &&
                 position.col < int(std::size(spreadsheet_[position.row]))
             ? spreadsheet_[position.row][position.col].get()
             : nullptr;
}

const Cell* Sheet::GetCell(Position position) const {
  const Cell* result = GetCell(position);
  return result;
}

void Sheet::ClearCell(Position position) {
  if (!position.IsValid()) {
    throw InvalidPositionException("Position is not valid.");
  }

  if (position.row < int(std::size(spreadsheet_)) &&
      position.col < int(std::size(spreadsheet_[position.row]))) {
    if (spreadsheet_[position.row][position.col]) {
      spreadsheet_[position.row][position.col]->Clear();

      if (!spreadsheet_[position.row][position.col]->IsReferenced()) {
        spreadsheet_[position.row][position.col].reset();
      }
    }
  }
}

Size Sheet::GetPrintableSize() const {
  Size size;

  for (int row = 0; row < int(std::size(spreadsheet_)); ++row) {
    for (int col = 0; col < int(std::size(spreadsheet_[row])); ++col) {
      if (spreadsheet_[row][col]) {
        if (!spreadsheet_[row][col]->GetText().empty()) {
          size.rows = std::max(size.rows, row + 1);
          size.cols = std::max(size.cols, col + 1);
        }
      }
    }
  }

  return size;
}

void Sheet::PrintValues(std::ostream& output) const {
  for (int row = 0; row < GetPrintableSize().rows; ++row) {
    for (int col = 0; col < GetPrintableSize().cols; ++col) {
      if (col > 0) {
        output << '\t';
      }

      if (col < int(std::size(spreadsheet_[row]))) {
        if (spreadsheet_[row][col]) {
          std::visit([&output](const auto& value) { output << value; },
                     spreadsheet_[row][col]->GetValue());
        }
      }
    }

    output << '\n';
  }
}

void Sheet::PrintTexts(std::ostream& output) const {
  for (int row = 0; row < GetPrintableSize().rows; ++row) {
    for (int col = 0; col < GetPrintableSize().cols; ++col) {
      if (col) {
        output << '\t';
      }

      if (col < int(std::size(spreadsheet_[row]))) {
        if (spreadsheet_[row][col]) {
          output << spreadsheet_[row][col]->GetText();
        }
      }
    }

    output << '\n';
  }
}

std::unique_ptr<SheetInterface> CreateSheet() {
  return std::make_unique<Sheet>();
}