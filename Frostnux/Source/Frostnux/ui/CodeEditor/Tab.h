#pragma once
#include "TextBuffer.h"
#include "Highlighter.h"
#include <deque>

namespace Frostnux {

	class Selection
	{
	public:
		void set(Position anchor, Position active) { anchor_ = anchor; active_ = active; }
		void clear(Position p) { anchor_ = active_ = p; }

		[[nodiscard]] bool		empty()  const { return anchor_ == active_; }
		[[nodiscard]] Position	start()  const { return std::min(anchor_, active_); }
		[[nodiscard]] Position	end()    const { return std::max(anchor_, active_); }
		[[nodiscard]] Position	anchor() const { return anchor_; }
		[[nodiscard]] Position	active() const { return active_; }

		void setActive(Position p) { active_ = p; }
		void setAnchor(Position p) { anchor_ = p; }
	private:
		Position anchor_, active_;
	};

	struct Edit
	{
		Position		 from;
		Position		 to;
		std::u32string	 removed;
		std::u32string	 inserted;
	};

	class UndoStack
	{
	public:
		void push(Edit e) { undo_.push_back(std::move(e)); redo_.clear(); }
		[[nodiscard]] bool canUndo() const { return !undo_.empty(); }
		[[nodiscard]] bool canRedo() const { return !redo_.empty(); }

		Edit popUndo()
		{
			Edit e = std::move(undo_.back()); undo_.pop_back();
			redo_.push_back(e);
			return e;
		}
		Edit popRedo()
		{
			Edit e = std::move(redo_.back()); redo_.pop_back();
			undo_.push_back(e);
			return e;
		}
		void clear() { undo_.clear(); redo_.clear(); }

	private:
		std::deque<Edit> undo_, redo_;
	};

	class ScrollBar
	{
	public:
		enum class Orientation { Vertical, Horizontal };

		explicit ScrollBar(Orientation o = Orientation::Vertical) : orient_(o) {}

		void setContent(double content, double viewport)
		{
			content_ = content;
			viewport_ = viewport;
			value_ = std::clamp(value_, 0.0, maxScroll());
			updateThumb();
		}
		void setValue(double v)
		{
			value_ = std::clamp(v, 0.0, maxScroll());
			updateThumb();
		}
		[[nodiscard]] double value()     const { return value_; }
		[[nodiscard]] double maxScroll() const { return std::max(0.0, content_ - viewport_); }

		void layout(double x, double y, double w, double h)
		{
			trackX_ = x; trackY_ = y; trackW_ = w; trackH_ = h;
			updateThumb();
		}

		[[nodiscard]] bool hitTest(double x, double y) const
		{
			return x >= trackX_ && x <= trackX_ + trackW_ &&
				y >= trackY_ && y <= trackY_ + trackH_;
		}

		bool onMouseDown(double x, double y)
		{
			if (!hitTest(x, y)) return false;
			double p = (orient_ == Orientation::Vertical) ? (y - trackY_) : (x - trackX_);
			if (p >= thumbPos_ && p <= thumbPos_ + thumbLen_)
			{
				dragging_ = true;
				dragOffset_ = p - thumbPos_;
			}
			else
			{
				double page = viewport_ * (p < thumbPos_ ? -1 : 1);
				setValue(value_ + page);
				dragging_ = true;
				dragOffset_ = thumbLen_ / 2;
			}
			return true;
		}

		bool onMouseMove(double x, double y)
		{
			if (!dragging_) return false;
			double trackLen = (orient_ == Orientation::Vertical) ? trackH_ : trackW_;
			double range = trackLen - thumbLen_;
			if (range <= 0) return true;
			double p = (orient_ == Orientation::Vertical) ? (y - trackY_) : (x - trackX_);
			double t = std::clamp((p - dragOffset_) / range, 0.0, 1.0);
			value_ = t * maxScroll();
			updateThumb();
			return true;
		}

		void onMouseUp() { dragging_ = false; }

		[[nodiscard]] double trackX()   const { return trackX_; }
		[[nodiscard]] double trackY()   const { return trackY_; }
		[[nodiscard]] double trackW()   const { return trackW_; }
		[[nodiscard]] double trackH()   const { return trackH_; }
		[[nodiscard]] double thumbPos() const { return thumbPos_; }
		[[nodiscard]] double thumbLen() const { return thumbLen_; }
	private:
		void updateThumb()
		{
			double trackLen = (orient_ == Orientation::Vertical) ? trackH_ : trackW_;
			if (content_ <= viewport_ || content_ <= 0)
			{
				thumbLen_ = trackLen; thumbPos_ = 0; return;
			}
			thumbLen_ = std::max(24.0, trackLen * viewport_ / content_);
			double range = trackLen - thumbLen_;
			double t = (maxScroll() > 0) ? value_ / maxScroll() : 0;
			thumbPos_ = t * range;
		}

		Orientation orient_;
		double content_ = 0, viewport_ = 0, value_ = 0;
		double trackX_ = 0, trackY_ = 0, trackW_ = 0, trackH_ = 0;
		double thumbPos_ = 0, thumbLen_ = 0;
		double dragOffset_ = 0;
		bool   dragging_ = false;
	};

	struct Tab
	{
		std::u32string	title = U"untitled";
		std::string		path;
		bool			dirty = false;

		TextBuffer	 buffer;
		Selection	 selection;
		UndoStack	 undo;
		Highlighter	 highlighter;

		double scrollX = 0;
		double scrollY = 0;

		ScrollBar vbar { ScrollBar::Orientation::Vertical };
		ScrollBar hbar { ScrollBar::Orientation::Horizontal };

		int		 desiredCol = 0;
		double	 longestWidth = 0;
		bool	 longestDirty = true;

		void invalidateHighlight();
	};

}
