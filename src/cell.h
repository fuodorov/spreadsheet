#pragma once

#include <functional>
#include <optional>
#include <set>
#include <stack>
#include <unordered_set>

#include "common.h"
#include "formula.h"

class Sheet;

class Cell : public CellInterface {
 public:
  Cell(Sheet& sheet);
  ~Cell();

  void Set(std::string content, Position pos, Sheet* sheet);
  void Clear();

  Value GetValue() const override;
  std::string GetText() const override;

  std::vector<Position> GetReferencedCells() const override;

  bool IsReferenced() const;
  void ClearCache();

 private:
  class Impl;

  bool IsLoop(Cell* cell, std::unordered_set<Cell*>& cells,
              const Position position);
  bool FindLoop(const Impl& impl, Position position);

  class Impl {
   public:
    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const;

    virtual bool IsEmptyCache();
    virtual void ClearCache();

    virtual ~Impl() = default;
  };

  class EmptyImpl : public Impl {
   public:
    Value GetValue() const override;
    std::string GetText() const override;
  };

  class TextImpl : public Impl {
   public:
    explicit TextImpl(std::string content);
    Value GetValue() const override;
    std::string GetText() const override;

   private:
    std::string text_;
  };

  class FormulaImpl : public Impl {
   public:
    explicit FormulaImpl(std::string content, SheetInterface& sheet);

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsEmptyCache() override;
    void ClearCache() override;

   private:
    mutable std::optional<FormulaInterface::Value> db_;
    std::unique_ptr<FormulaInterface> formula_;
    SheetInterface& sheet_;
  };

  std::unique_ptr<Impl> impl_;
  Sheet& sheet_;

  std::unordered_set<Cell*> calc_cells_;
  std::unordered_set<Cell*> use_cells_;
  Position pos_;
};