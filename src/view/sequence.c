#include "sequence.h"
#include "../mused.h"
#include "gui/bevel.h"
#include "../mybevdefs.h"
#include "action.h"
#include "gui/mouse.h"

void sequence_view_inner(SDL_Surface *dest_surface, const SDL_Rect *_dest, const SDL_Event *event)
{
	SDL_Rect dest;
	copy_rect(&dest, _dest);
	
	bevel(mused.screen, _dest, mused.slider_bevel->surface, BEV_SEQUENCE_BORDER);

	const int height = 10;
	const int top = mused.sequence_position;
	const int w = my_max(dest.w / mused.song.num_channels - 1, 24);
	int h = dest.h;
	const int bottom = top + h * mused.sequenceview_steps / height - 1;
	
	if (mused.song.num_channels * (w + 1) > dest.w)
		h -= SCROLLBAR;
		
	if (mused.current_sequencepos >= top && mused.current_sequencepos < bottom)
	{
		SDL_Rect sel = {dest.x, dest.y + (mused.current_sequencepos * height / mused.sequenceview_steps - top * height / mused.sequenceview_steps) + 1, dest.w, height};
		
		clip_rect(&sel, &dest);
		SDL_SetClipRect(mused.screen, &sel);
		bevel(dest_surface, &sel, mused.slider_bevel->surface, BEV_SELECTED_SEQUENCE_ROW);
	}
	
	slider_set_params(&mused.sequence_slider_param, 0, mused.song.song_length - mused.sequenceview_steps, my_max(0, top), (bottom / mused.sequenceview_steps - 1) * mused.sequenceview_steps, &mused.sequence_position, mused.sequenceview_steps, SLIDER_VERTICAL, mused.slider_bevel->surface);
	
	int vischans = my_min(mused.song.num_channels - 1, mused.sequence_horiz_position + (dest.w - (w - 1)) / (w + 1));
	
	slider_set_params(&mused.sequence_horiz_slider_param, 0, mused.song.num_channels - 1, mused.sequence_horiz_position, vischans, 
		&mused.sequence_horiz_position, 1, SLIDER_HORIZONTAL, mused.slider_bevel->surface);
		
	SDL_Rect clip2;
	copy_rect(&clip2, &dest);
	clip2.h += 8;
	clip2.y -= 4;
		
	for (int channel = mused.sequence_horiz_position ; channel < mused.song.num_channels && channel <= vischans ; ++channel)
	{
		const MusSeqPattern *sp = &mused.song.sequence[channel][0];
		const int x = (channel - mused.sequence_horiz_position) * (w + 1);
		
		for (int i = 0 ; i < mused.song.num_sequences[channel] ; ++i, ++sp)
		{
			if (sp->position >= bottom) break;
			
			int len = mused.song.pattern[sp->pattern].num_steps;
			
			if (i < mused.song.num_sequences[channel] - 1)
				len = my_min(len, (sp + 1)->position - sp->position);
			
			if (sp->position + len <= top) continue;
			
			SDL_Rect pat = { x + dest.x + 1, (sp->position - top) * height / mused.sequenceview_steps + dest.y + 1, w, len * height / mused.sequenceview_steps };
			SDL_Rect text;
			
			copy_rect(&text, &pat);
			clip_rect(&text, &dest);
			
			SDL_SetClipRect(mused.screen, &dest);
			
			int bev = (mused.current_sequencetrack == channel && mused.current_sequencepos >= sp->position && mused.current_sequencepos < sp->position + len) ? BEV_PATTERN_CURRENT : BEV_PATTERN;
			
			clip_rect(&pat, &clip2);
			bevel(mused.screen, &pat, mused.slider_bevel->surface, bev);
			
			check_event(event, &pat, select_sequence_position, MAKEPTR(channel), MAKEPTR(sp->position), 0);
			
			adjust_rect(&text, 2);
			text.h += 1;
			
			SDL_SetClipRect(mused.screen, &text);
			
			font_write_args(&mused.tinyfont_sequence_normal, mused.screen, &text, "%02X%+d", sp->pattern, sp->note_offset);
		}
	}
	
	SDL_SetClipRect(mused.screen, &dest);
	
	SDL_Rect loop = { dest.x, (mused.song.loop_point - top) * height / mused.sequenceview_steps + dest.y + 1, dest.w, (mused.song.song_length - mused.song.loop_point) * height / mused.sequenceview_steps };
	bevel(mused.screen, &loop, mused.slider_bevel->surface, BEV_SEQUENCE_LOOP);
	
	if (mused.focus == EDITSEQUENCE)
	{
		if (mused.current_sequencepos >= top  && mused.current_sequencepos < bottom && mused.current_sequencetrack >= mused.sequence_horiz_position && mused.current_sequencetrack <= vischans) 
		{
			SDL_Rect pat = { (mused.current_sequencetrack - mused.sequence_horiz_position) * (w + 1) + dest.x, (mused.current_sequencepos - top) * height / mused.sequenceview_steps + dest.y, w, height };
				
			clip_rect(&pat, &dest);
			adjust_rect(&pat, -2);
			pat.x += 1;
			pat.y += 1;
			
			set_cursor(&pat);
		}
		else
		{
			set_cursor(NULL);
		}
		
		if (mused.selection.start != mused.selection.end)
		{
			if (mused.selection.start <= bottom && mused.selection.end >= top)
			{
				SDL_Rect selection = { dest.x + (w + 1) * (mused.current_sequencetrack - mused.sequence_horiz_position), 
					dest.y + height * (mused.selection.start - mused.sequence_position) / mused.sequenceview_steps, w, height * (mused.selection.end - mused.selection.start) / mused.sequenceview_steps};
					
				adjust_rect(&selection, -3);
				bevel(mused.screen, &selection, mused.slider_bevel->surface, BEV_SELECTION);
			}
		}
	}
	
	if (mused.flags & SONG_PLAYING)
    {
		SDL_Rect play = { dest.x, (mused.stat_song_position - top) * height / mused.sequenceview_steps + dest.y, dest.w, 2 };
		clip_rect(&play, &dest);
        bevel(mused.screen,&play, mused.slider_bevel->surface, BEV_SEQUENCE_PLAY_POS);
    }

	
	SDL_SetClipRect(mused.screen, NULL);
	
	check_mouse_wheel_event(event, _dest, &mused.sequence_slider_param);
}


static void sequence_view_stepcounter(SDL_Surface *dest_surface, const SDL_Rect *_dest, const SDL_Event *event)
{
	const int height = 10;
	const int top = mused.sequence_position;
	
	SDL_SetClipRect(mused.screen, _dest);
	
	bevel(dest_surface, _dest, mused.slider_bevel->surface, BEV_SEQUENCE_BORDER);
	
	SDL_Rect dest;
	copy_rect(&dest, _dest);
	adjust_rect(&dest, 1);
	dest.w += 1;
	
	SDL_Rect pos = { dest.x + 1, dest.y + 2, dest.w - 1, height };
	
	if (mused.current_sequencepos >= top) 
	{
		SDL_Rect sel = { dest.x + 1, dest.y + 2 + (mused.current_sequencepos - top) * height / mused.sequenceview_steps , dest.w -1, height };
		sel.x -= 1;
		sel.y -= 2;
		SDL_SetClipRect(mused.screen, &sel);
		bevel(dest_surface, &sel, mused.slider_bevel->surface, BEV_SELECTED_SEQUENCE_ROW);
	}
		
	
	for (int p = top ; pos.y < dest.y + dest.h ; p += mused.sequenceview_steps, pos.y += height)
	{
		
		clip_rect(&pos, &dest);
		SDL_SetClipRect(mused.screen, &pos);
	
		if (SHOW_DECIMALS & mused.flags)
			font_write_args(&mused.tinyfont_sequence_counter, mused.screen, &pos, "%04d", p % 10000);
		else
			font_write_args(&mused.tinyfont_sequence_counter, mused.screen, &pos, "%04X", p & 0xffff);
	
		
	}
	
	SDL_SetClipRect(mused.screen, NULL);
}


void sequence_view2(SDL_Surface *dest_surface, const SDL_Rect *_dest, const SDL_Event *event, void *param)
{
	SDL_Rect seq, pos;
	copy_rect(&seq, _dest);
	copy_rect(&pos, _dest);
		
	seq.w -= mused.tinyfont.w * 4 + 4;
	seq.x += mused.tinyfont.w * 4 + 4;
	
	pos.w = mused.tinyfont.w * 4 + 4;
		
	sequence_view_stepcounter(dest_surface, &pos, event);
	sequence_view_inner(dest_surface, &seq, event);
	
	const int w = my_max(_dest->w / mused.song.num_channels - 1, 64);

	if (mused.song.num_channels * (w + 1) > _dest->w)
	{
		SDL_Rect scrollbar = { _dest->x, _dest->y + _dest->h - SCROLLBAR, _dest->w, SCROLLBAR };
		
		slider(dest_surface, &scrollbar, event, &mused.sequence_horiz_slider_param); 
	}	
}
