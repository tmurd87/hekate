/*
 * Copyright (c) 2018-2019 CTCaer
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#include "gui.h"
#include "../config/config.h"
#include "../config/ini.h"
#include "../gfx/di.h"
#include "../libs/lvgl/lvgl.h"
#include "../mem/heap.h"
#include "../storage/nx_sd.h"
#include "../utils/list.h"
#include "../utils/sprintf.h"
#include "../utils/types.h"

extern hekate_config h_cfg;
extern nyx_config n_cfg;

static lv_obj_t *autoboot_btn;
static bool autoboot_first_time = true;

static lv_res_t auto_hos_poweroff_toggle(lv_obj_t *btn)
{
	h_cfg.autohosoff = !h_cfg.autohosoff;

	if (!h_cfg.autohosoff)
		lv_btn_set_state(btn, LV_BTN_STATE_REL);
	else
		lv_btn_set_state(btn, LV_BTN_STATE_TGL_REL);

	nyx_generic_onoff_toggle(btn);

	return LV_RES_OK;
}

static lv_res_t auto_nogc_toggle(lv_obj_t *btn)
{
	h_cfg.autonogc = !h_cfg.autonogc;

	if (!h_cfg.autonogc)
		lv_btn_set_state(btn, LV_BTN_STATE_REL);
	else
		lv_btn_set_state(btn, LV_BTN_STATE_TGL_REL);

	nyx_generic_onoff_toggle(btn);

	return LV_RES_OK;
}

static lv_res_t _win_autoboot_close_action(lv_obj_t * btn)
{
	if (!h_cfg.autoboot)
		lv_btn_set_state(autoboot_btn, LV_BTN_STATE_REL);
	else
		lv_btn_set_state(autoboot_btn, LV_BTN_STATE_TGL_REL);

	nyx_generic_onoff_toggle(autoboot_btn);

	lv_obj_t * win = lv_win_get_from_btn(btn);

	lv_obj_del(win);

	close_btn = NULL;

	return LV_RES_INV;
}

lv_obj_t *create_window_autoboot(const char *win_title)
{
	static lv_style_t win_bg_style;

	lv_style_copy(&win_bg_style, &lv_style_plain);
	win_bg_style.body.main_color = LV_COLOR_HEX(0x2D2D2D);// TODO: COLOR_HOS_BG
	win_bg_style.body.grad_color = win_bg_style.body.main_color;

	lv_obj_t *win = lv_win_create(lv_scr_act(), NULL);
	lv_win_set_title(win, win_title);
	lv_win_set_style(win, LV_WIN_STYLE_BG, &win_bg_style);
	lv_obj_set_size(win, LV_HOR_RES, LV_VER_RES);

	close_btn = lv_win_add_btn(win, NULL, SYMBOL_CLOSE" Close", _win_autoboot_close_action);

	return win;
}

// TODO: instant update of button for these.
static lv_res_t _autoboot_disable_action(lv_obj_t *btn)
{
	h_cfg.autoboot = 0;
	h_cfg.autoboot_list = 0;

	lv_btn_set_state(autoboot_btn, LV_BTN_STATE_REL);
	nyx_generic_onoff_toggle(autoboot_btn);

	lv_obj_t * win = lv_win_get_from_btn(btn);

	lv_obj_del(win);

	return LV_RES_OK;
}

lv_obj_t *auto_main_list;
lv_obj_t *auto_more_list;
static lv_res_t _autoboot_enable_main_action(lv_obj_t *btn)
{
	h_cfg.autoboot = lv_list_get_btn_index(auto_main_list, btn) + 1;
	h_cfg.autoboot_list = 0;

	lv_btn_set_state(autoboot_btn, LV_BTN_STATE_TGL_REL);
	nyx_generic_onoff_toggle(autoboot_btn);

	lv_obj_t *obj = lv_obj_get_parent(btn);
	for (int i = 0; i < 5; i++)
		obj = lv_obj_get_parent(obj);
	lv_obj_del(obj);

	return LV_RES_INV;
}

static lv_res_t _autoboot_enable_more_action(lv_obj_t *btn)
{
	h_cfg.autoboot = lv_list_get_btn_index(auto_more_list, btn) + 1;
	h_cfg.autoboot_list = 1;

	lv_btn_set_state(autoboot_btn, LV_BTN_STATE_TGL_REL);
	nyx_generic_onoff_toggle(autoboot_btn);

	lv_obj_t *obj = lv_obj_get_parent(btn);
	for (int i = 0; i < 5; i++)
		obj = lv_obj_get_parent(obj);
	lv_obj_del(obj);

	return LV_RES_INV;
}

static void _create_autoboot_window()
{
	lv_obj_t *win = create_window_autoboot(SYMBOL_GPS" Auto Boot");
	lv_win_add_btn(win, NULL, SYMBOL_POWER" Disable", _autoboot_disable_action);

	static lv_style_t h_style;
	lv_style_copy(&h_style, &lv_style_transp);
	h_style.body.padding.inner = 0;
	h_style.body.padding.hor = LV_DPI - (LV_DPI / 4);
	h_style.body.padding.ver = LV_DPI / 6;

	// Main configurations container.
	lv_obj_t *h1 = lv_cont_create(win, NULL);
	lv_cont_set_style(h1, &h_style);
	lv_cont_set_fit(h1, false, true);
	lv_obj_set_width(h1, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(h1, false);
	lv_cont_set_layout(h1, LV_LAYOUT_OFF);

	lv_obj_t *label_sep = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt = lv_label_create(h1, NULL);
	lv_label_set_static_text(label_txt, "Main configurations");
	lv_obj_set_style(label_txt, lv_theme_get_current()->label.prim);
	lv_obj_align(label_txt, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -(LV_DPI / 4));

	lv_obj_t *line_sep = lv_line_create(h1, NULL);
	static const lv_point_t line_pp[] = { {0, 0}, { LV_HOR_RES - (LV_DPI - (LV_DPI / 4)) * 2, 0} };
	lv_line_set_points(line_sep, line_pp, 2);
	lv_line_set_style(line_sep, lv_theme_get_current()->line.decor);
	lv_obj_align(line_sep, label_txt, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 8);

	// Create list and populate it with Main boot entries.
	lv_obj_t *list_main = lv_list_create(h1, NULL);
	auto_main_list = list_main;
	lv_obj_align(list_main, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);

	lv_obj_set_size(list_main, LV_HOR_RES * 4 / 10, LV_VER_RES * 4 / 7);
	lv_list_set_single_mode(list_main, true);

	sd_mount();

	LIST_INIT(ini_sections);
	if (ini_parse(&ini_sections, "bootloader/hekate_ipl.ini", false))
	{
		LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_sections, link)
		{
			if (!strcmp(ini_sec->name, "config") || (ini_sec->type != INI_CHOICE))
				continue;

			lv_list_add(list_main, NULL, ini_sec->name, _autoboot_enable_main_action);
		}
	}

	// More configuration container.
	lv_obj_t *h2 = lv_cont_create(win, NULL);
	lv_cont_set_style(h2, &h_style);
	lv_cont_set_fit(h2, false, true);
	lv_obj_set_width(h2, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(h2, false);
	lv_cont_set_layout(h2, LV_LAYOUT_OFF);
	lv_obj_align(h2, h1, LV_ALIGN_OUT_RIGHT_TOP, LV_DPI * 17 / 29, 0);

	label_sep = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *label_txt3 = lv_label_create(h2, NULL);
	lv_label_set_static_text(label_txt3, "Ini folder configurations");
	lv_obj_set_style(label_txt3, lv_theme_get_current()->label.prim);
	lv_obj_align(label_txt3, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI / 11);

	line_sep = lv_line_create(h2, line_sep);
	lv_obj_align(line_sep, label_txt3, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 2), LV_DPI / 8);
	lv_line_set_style(line_sep, lv_theme_get_current()->line.decor);

	// Create list and populate it with more cfg boot entries.
	lv_obj_t *list_more_cfg = lv_list_create(h2, NULL);
	auto_more_list = list_more_cfg;
	lv_obj_align(list_more_cfg, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 2, LV_DPI / 4);

	lv_obj_set_size(list_more_cfg, LV_HOR_RES * 4 / 10, LV_VER_RES * 4 / 7);
	lv_list_set_single_mode(list_more_cfg, true);

	LIST_INIT(ini_list_sections);
	if (ini_parse(&ini_list_sections, "bootloader/ini", true))
	{
		LIST_FOREACH_ENTRY(ini_sec_t, ini_sec, &ini_list_sections, link)
		{
			if (!strcmp(ini_sec->name, "config") || (ini_sec->type != INI_CHOICE))
				continue;

			lv_list_add(list_more_cfg, NULL, ini_sec->name, _autoboot_enable_more_action);
		}
	}

	sd_unmount(false);
}

static lv_res_t _autoboot_hide_delay_action(lv_obj_t *btn)
{
	if (!autoboot_first_time)
		_create_autoboot_window();

	if (!h_cfg.autoboot && autoboot_first_time)
		lv_btn_set_state(btn, LV_BTN_STATE_REL);
	else
		lv_btn_set_state(btn, LV_BTN_STATE_TGL_REL);
	autoboot_first_time = false;

	nyx_generic_onoff_toggle(btn);

	return LV_RES_INV;
}

static lv_res_t _autoboot_delay_action(lv_obj_t *ddlist)
{
	h_cfg.bootwait = lv_ddlist_get_selected(ddlist);

	return LV_RES_OK;
}

static lv_res_t _slider_brightness_action(lv_obj_t * slider)
{
	display_backlight_brightness(lv_slider_get_value(slider) - 20, 0);
	h_cfg.backlight = lv_slider_get_value(slider);

	return LV_RES_OK;
}

static lv_res_t _data_verification_action(lv_obj_t *ddlist)
{
	n_cfg.verification = lv_ddlist_get_selected(ddlist);

	return LV_RES_OK;
}

void create_flat_button(lv_obj_t *parent, lv_obj_t *btn, lv_color_t color, lv_action_t action)
{
	lv_style_t *btn_onoff_rel_hos_style = malloc(sizeof(lv_style_t));
	lv_style_t *btn_onoff_pr_hos_style = malloc(sizeof(lv_style_t));
	lv_style_copy(btn_onoff_rel_hos_style, lv_theme_get_current()->btn.rel);
	btn_onoff_rel_hos_style->body.main_color = color;
	btn_onoff_rel_hos_style->body.grad_color = btn_onoff_rel_hos_style->body.main_color;
	btn_onoff_rel_hos_style->body.padding.hor = 0;
	btn_onoff_rel_hos_style->body.radius = 0;

	lv_style_copy(btn_onoff_pr_hos_style, lv_theme_get_current()->btn.pr);
	btn_onoff_pr_hos_style->body.main_color = color;
	btn_onoff_pr_hos_style->body.grad_color = btn_onoff_pr_hos_style->body.main_color;
	btn_onoff_pr_hos_style->body.padding.hor = 0;
	btn_onoff_pr_hos_style->body.border.color = LV_COLOR_GRAY;
	btn_onoff_pr_hos_style->body.border.width = 4;
	btn_onoff_pr_hos_style->body.radius = 0;

	lv_btn_set_style(btn, LV_BTN_STYLE_REL, btn_onoff_rel_hos_style);
	lv_btn_set_style(btn, LV_BTN_STYLE_PR, btn_onoff_pr_hos_style);
	lv_btn_set_style(btn, LV_BTN_STYLE_TGL_REL, btn_onoff_rel_hos_style);
	lv_btn_set_style(btn, LV_BTN_STYLE_TGL_PR, btn_onoff_pr_hos_style);

	lv_btn_set_fit(btn, false, true);
	lv_obj_set_width(btn, lv_obj_get_height(btn));
	lv_btn_set_toggle(btn, true);

	if (action)
		lv_btn_set_action(btn, LV_BTN_ACTION_CLICK, action);
}

typedef struct _color_test_ctxt
{
	u16 hue;
	lv_obj_t *label;
	lv_obj_t *icons;
	lv_obj_t *slider;
	lv_obj_t *button;
	lv_obj_t *hue_slider;
	lv_obj_t *hue_label;
} color_test_ctxt;

color_test_ctxt color_test;

static lv_res_t _save_theme_color_action(lv_obj_t *btn)
{
	n_cfg.themecolor = color_test.hue;

	// Save nyx config.
	create_nyx_config_entry();

	reload_nyx();

	return LV_RES_OK;
}

static void _test_nyx_color(u16 hue)
{
	lv_color_t color = lv_color_hsv_to_rgb(hue, 100, 100);
	static lv_style_t btn_tgl_test;
	lv_style_copy(&btn_tgl_test, lv_btn_get_style(color_test.button, LV_BTN_STATE_TGL_PR));
	btn_tgl_test.body.border.color = color;
	btn_tgl_test.text.color = color;
	lv_btn_set_style(color_test.button, LV_BTN_STATE_TGL_PR, &btn_tgl_test);

	static lv_style_t txt_test;
	lv_style_copy(&txt_test, lv_label_get_style(color_test.label));
	txt_test.text.color = color;
	lv_obj_set_style(color_test.label, &txt_test);
	lv_obj_set_style(color_test.icons, &txt_test);

	static lv_style_t slider_test, slider_ind;
	lv_style_copy(&slider_test, lv_slider_get_style(color_test.slider, LV_SLIDER_STYLE_KNOB));
	lv_style_copy(&slider_ind, lv_slider_get_style(color_test.slider, LV_SLIDER_STYLE_INDIC));
	slider_test.body.main_color = color;
	slider_test.body.grad_color = slider_test.body.main_color;
	slider_ind.body.main_color = lv_color_hsv_to_rgb(hue, 100, 72);
	slider_ind.body.grad_color = slider_ind.body.main_color;
	lv_slider_set_style(color_test.slider, LV_SLIDER_STYLE_KNOB, &slider_test);
	lv_slider_set_style(color_test.slider, LV_SLIDER_STYLE_INDIC, &slider_ind);
}

static lv_res_t _slider_hue_action(lv_obj_t *slider)
{
	if (color_test.hue != lv_slider_get_value(slider))
	{
		color_test.hue = lv_slider_get_value(slider);
		_test_nyx_color(color_test.hue);
		char hue[8];
		s_printf(hue, "%03d", color_test.hue);
		lv_label_set_text(color_test.hue_label, hue);
	}

	return LV_RES_OK;
}

static lv_res_t _preset_hue_action(lv_obj_t *btn)
{
	lv_btn_ext_t *ext = lv_obj_get_ext_attr(btn);

	if (color_test.hue != ext->idx)
	{
		color_test.hue = ext->idx;
		_test_nyx_color(color_test.hue);
		char hue[8];
		s_printf(hue, "%03d", color_test.hue);
		lv_label_set_text(color_test.hue_label, hue);
		lv_bar_set_value(color_test.hue_slider, color_test.hue);
	}

	return LV_RES_OK;
}

const u16 theme_colors[17] = {
	4, 13, 23, 33, 43, 54, 66, 89, 124, 167, 187, 200, 208, 231, 261, 291, 341
};

static lv_res_t _create_window_nyx_colors(lv_obj_t *btn)
{
	lv_obj_t *win = nyx_create_standard_window(SYMBOL_COPY" Choose a Nyx Color Theme");
	lv_win_add_btn(win, NULL, SYMBOL_SAVE" Save & Reload", _save_theme_color_action);

	// Set current color.
	color_test.hue = n_cfg.themecolor;

	lv_obj_t *sep = lv_label_create(win, NULL);
	lv_label_set_static_text(sep, "");
	lv_obj_align(sep, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);

	// Create container to keep content inside.
	lv_obj_t *h1 = lv_cont_create(win, NULL);
	lv_obj_set_size(h1, LV_HOR_RES - (LV_DPI * 8 / 10), LV_VER_RES / 7);

	// Create color preset buttons.
	lv_obj_t *color_btn = lv_btn_create(h1, NULL);
	lv_btn_ext_t *ext = lv_obj_get_ext_attr(color_btn);
	ext->idx = theme_colors[0];
	create_flat_button(h1, color_btn, lv_color_hsv_to_rgb(theme_colors[0], 100, 100), _preset_hue_action);
	lv_obj_t *color_btn2;

	for (u32 i = 1; i < 17; i++)
	{
		color_btn2 = lv_btn_create(h1, NULL);
		ext = lv_obj_get_ext_attr(color_btn2);
		ext->idx = theme_colors[i];
		create_flat_button(h1, color_btn2, lv_color_hsv_to_rgb(theme_colors[i], 100, 100), _preset_hue_action);
		lv_obj_align(color_btn2, color_btn, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
		color_btn = color_btn2;
	}

	lv_obj_align(h1, sep, LV_ALIGN_OUT_BOTTOM_MID, 0, LV_DPI / 4);

	// Create hue slider.
	lv_obj_t * slider = lv_slider_create(win, NULL);
	lv_obj_set_width(slider, 1070);
	lv_obj_set_height(slider, LV_DPI * 4 / 10);
	lv_bar_set_range(slider, 0, 359);
	lv_bar_set_value(slider, color_test.hue);
	lv_slider_set_action(slider, _slider_hue_action);
	lv_obj_align(slider, h1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
	color_test.hue_slider = slider;

	// Create hue label.
	lv_obj_t *hue_text_label = lv_label_create(win, NULL);
	lv_obj_align(hue_text_label, slider, LV_ALIGN_OUT_RIGHT_MID, LV_DPI * 24 / 100, 0);
	char hue[8];
	s_printf(hue, "%03d", color_test.hue);
	lv_label_set_text(hue_text_label, hue);
	color_test.hue_label = hue_text_label;

	// Create sample text.
	lv_obj_t *h2 = lv_cont_create(win, NULL);
	lv_obj_set_size(h2, LV_HOR_RES - (LV_DPI * 8 / 10), LV_VER_RES / 3);
	lv_obj_align(h2, slider, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI);

	lv_obj_t *lbl_sample = lv_label_create(h2, NULL);
	lv_label_set_static_text(lbl_sample, "Sample:");

	lv_obj_t *lbl_test = lv_label_create(h2, NULL);
	lv_label_set_long_mode(lbl_test, LV_LABEL_LONG_BREAK);
	lv_label_set_static_text(lbl_test,
		"Lorem ipsum dolor sit amet, consectetur adipisicing elit, "
		"sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
		"Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris "
		"nisi ut aliquip ex ea commodo consequat.");
	lv_obj_set_width(lbl_test, lv_obj_get_width(h2) - LV_DPI * 6 / 10);
	lv_obj_align(lbl_test, lbl_sample, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 5);
	color_test.label = lbl_test;

	// Create sample icons.
	lv_obj_t *lbl_icons = lv_label_create(h2, NULL);
	lv_label_set_static_text(lbl_icons,
		SYMBOL_BRIGHTNESS SYMBOL_CHARGE SYMBOL_FILE SYMBOL_DRIVE SYMBOL_FILE_CODE
		SYMBOL_EDIT SYMBOL_HINT SYMBOL_DRIVE SYMBOL_KEYBOARD SYMBOL_POWER);
	lv_obj_align(lbl_icons, lbl_test, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI * 2 / 5);
	color_test.icons = lbl_icons;

	// Create sample slider.
	lv_obj_t *slider_test = lv_slider_create(h2, NULL);
	lv_obj_align(slider_test, lbl_test, LV_ALIGN_OUT_BOTTOM_MID, 0, LV_DPI * 2 / 5);
	lv_obj_set_click(slider_test, false);
	lv_bar_set_value(slider_test, 60);
	color_test.slider = slider_test;

	// Create sample button.
	lv_obj_t *btn_test = lv_btn_create(h2, NULL);
	lv_btn_set_state(btn_test, LV_BTN_STATE_TGL_PR);
	lv_obj_align(btn_test, lbl_test, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, LV_DPI / 5);
	lv_label_create(btn_test, NULL);
	lv_obj_set_click(btn_test, false);
	color_test.button = btn_test;

	_test_nyx_color(color_test.hue);

	return LV_RES_OK;
}

lv_obj_t *epoch_ta;
static lv_res_t _create_mbox_clock_edit_action(lv_obj_t * btns, const char * txt)
{
	int btn_idx = lv_btnm_get_pressed(btns);

	if (btn_idx == 1)
		n_cfg.timeoff = strtol(lv_ta_get_text(epoch_ta), NULL, 16);

	mbox_action(btns, txt);

	return LV_RES_INV;
}

static lv_res_t _create_mbox_clock_edit(lv_obj_t *btn)
{
	lv_style_t *darken;
	darken = malloc(sizeof(lv_style_t));
	lv_style_copy(darken, &lv_style_plain);
	darken->body.main_color = LV_COLOR_BLACK;
	darken->body.grad_color = darken->body.main_color;
	darken->body.opa = LV_OPA_30;

	lv_obj_t *dark_bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(dark_bg, darken);
	lv_obj_set_size(dark_bg, LV_HOR_RES, LV_VER_RES);

	static const char * mbox_btn_map[] = { "\211", "\222Done", "\222Cancel", "\211", "" };
	lv_obj_t *mbox = lv_mbox_create(dark_bg, NULL);
	lv_mbox_set_recolor_text(mbox, true);
	lv_obj_set_width(mbox, LV_HOR_RES / 9 * 6);

	lv_mbox_set_text(mbox, "Type the #C7EA46 epoch# offset in HEX s32:");

	lv_obj_t *ta = lv_ta_create(mbox, NULL);
	lv_obj_set_size(ta, LV_HOR_RES / 5, LV_VER_RES / 10);
	lv_obj_align(ta, NULL, LV_ALIGN_IN_TOP_RIGHT, -LV_DPI / 10, LV_DPI / 10);
	lv_ta_set_cursor_type(ta, LV_CURSOR_LINE);
	lv_ta_set_one_line(ta, true);
	epoch_ta = ta;

	static char epoch_off[16];
	s_printf(epoch_off, "%X", n_cfg.timeoff);
	lv_ta_set_text(ta, epoch_off);
	lv_ta_set_max_length(ta, 8);

	lv_obj_t *kb = lv_kb_create(mbox, NULL);
	lv_obj_set_size(kb, 2 * LV_HOR_RES / 3, LV_VER_RES / 3);
	lv_obj_align(kb, ta, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, LV_DPI);
	lv_kb_set_mode(kb, LV_KB_MODE_HEX);
	lv_kb_set_ta(kb, ta);
	lv_kb_set_cursor_manage(kb, true);

	lv_mbox_add_btns(mbox, mbox_btn_map, _create_mbox_clock_edit_action); // Important. After set_text.

	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

static lv_res_t _home_screen_action(lv_obj_t *ddlist)
{
	n_cfg.home_screen = lv_ddlist_get_selected(ddlist);

	return LV_RES_OK;
}

static lv_res_t _save_nyx_options_action(lv_obj_t *btn)
{
	static const char * mbox_btn_map[] = {"\211", "\222OK!", "\211", ""};
	lv_obj_t * mbox = lv_mbox_create(lv_scr_act(), NULL);
	lv_mbox_set_recolor_text(mbox, true);

	int res = !create_nyx_config_entry();

	if (res)
		lv_mbox_set_text(mbox, "#FF8000 Nyx Configuration#\n\n#96FF00 The configuration was saved to sd card!#");
	else
		lv_mbox_set_text(mbox, "#FF8000 Nyx Configuration#\n\n#FFDD00 Failed to save the configuration#\n#FFDD00 to sd card!#");
	lv_mbox_add_btns(mbox, mbox_btn_map, NULL);
	lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_top(mbox, true);

	return LV_RES_OK;
}

lv_res_t create_win_nyx_options(lv_obj_t *parrent_btn)
{
	lv_theme_t *th = lv_theme_get_current();

	lv_obj_t *win = nyx_create_standard_window(SYMBOL_HOME" Nyx Options");

	static lv_style_t h_style;
	lv_style_copy(&h_style, &lv_style_transp);
	h_style.body.padding.inner = 0;
	h_style.body.padding.hor = LV_DPI - (LV_DPI / 4);
	h_style.body.padding.ver = LV_DPI / 6;

	// Create containers to keep content inside.
	lv_obj_t * sw_h2 = lv_cont_create(win, NULL);
	lv_cont_set_style(sw_h2, &h_style);
	lv_cont_set_fit(sw_h2, false, true);
	lv_obj_set_width(sw_h2, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(sw_h2, false);
	lv_cont_set_layout(sw_h2, LV_LAYOUT_OFF);

	lv_obj_t * sw_h3 = lv_cont_create(win, NULL);
	lv_cont_set_style(sw_h3, &h_style);
	lv_cont_set_fit(sw_h3, false, true);
	lv_obj_set_width(sw_h3, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(sw_h3, false);
	lv_cont_set_layout(sw_h3, LV_LAYOUT_OFF);
	lv_obj_align(sw_h3, sw_h2, LV_ALIGN_OUT_RIGHT_TOP, LV_DPI * 11 / 25, 0);

	lv_obj_t * l_cont = lv_cont_create(sw_h2, NULL);
	lv_cont_set_style(l_cont, &lv_style_transp_tight);
	lv_cont_set_fit(l_cont, true, true);
	lv_obj_set_click(l_cont, false);
	lv_cont_set_layout(l_cont, LV_LAYOUT_OFF);
	lv_obj_set_opa_scale(l_cont, LV_OPA_40);

	lv_obj_t *label_sep = lv_label_create(sw_h2, NULL);
	lv_label_set_static_text(label_sep, "");

	lv_obj_t *btn = lv_btn_create(sw_h2, NULL);
	lv_obj_t *label_btn = lv_label_create(btn, NULL);
	lv_btn_set_fit(btn, true, true);
	lv_label_set_static_text(label_btn, SYMBOL_COPY" Color Theme");
	lv_obj_align(btn, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI / 5 + 3);
	lv_btn_set_action(btn, LV_BTN_ACTION_CLICK, _create_window_nyx_colors);

	lv_obj_t *label_txt2 = lv_label_create(sw_h2, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2, "Select a color for all #00FFC8 highlights# in Nyx.\n");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3 - 8);

	lv_obj_t *line_sep = lv_line_create(sw_h2, NULL);
	static const lv_point_t line_pp[] = { {0, 0}, { LV_HOR_RES - (LV_DPI - (LV_DPI / 4)) * 2, 0} };
	lv_line_set_points(line_sep, line_pp, 2);
	lv_line_set_style(line_sep, th->line.decor);
	lv_obj_align(line_sep, label_txt2, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 4);

	lv_obj_t *label_txt = lv_label_create(l_cont, NULL);
	lv_label_set_static_text(label_txt, SYMBOL_HOME" Home Screen");
	lv_obj_set_style(label_txt, th->label.prim);
	lv_obj_align(label_txt, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);

	lv_obj_t *ddlist = lv_ddlist_create(l_cont, NULL);
	lv_obj_set_top(ddlist, true);
	lv_ddlist_set_draw_arrow(ddlist, true);
	lv_ddlist_set_options(ddlist,
		"Main menu       \n"
		"All Configs\n"
		"Launch\n"
		"More Configs");
	lv_ddlist_set_selected(ddlist, n_cfg.home_screen);
	lv_ddlist_set_action(ddlist, _home_screen_action);
	lv_obj_align(ddlist, label_txt, LV_ALIGN_OUT_RIGHT_MID, LV_DPI * 2 / 3, 0);

	label_txt2 = lv_label_create(l_cont, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"Select what screen to show on Nyx boot.\n"
		"#FF8000 All Configs:# #C7EA46 Combines More configs into Launch empty slots.#\n"
		"#FF8000 Launch / More Configs:# #C7EA46 Uses the classic divided view.#");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, label_txt, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 4);

	line_sep = lv_line_create(sw_h2, line_sep);
	lv_obj_align(line_sep, label_txt2, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 4);

	lv_obj_t *btn2 = lv_btn_create(sw_h2, NULL);
	lv_obj_t *label_btn2 = lv_label_create(btn2, NULL);
	lv_btn_set_fit(btn2, true, true);
	lv_label_set_static_text(label_btn2, SYMBOL_CLOCK" Clock (HOS)");
	lv_obj_align(btn2, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);
	lv_btn_set_action(btn2, LV_BTN_ACTION_CLICK, _create_mbox_clock_edit); //TODO: Add support.
	lv_btn_set_state(btn2, LV_BTN_STATE_INA); //TODO: Add support.

	lv_obj_t *btn4 = lv_btn_create(sw_h2, btn2);
	label_btn2 = lv_label_create(btn4, NULL);
	lv_label_set_static_text(label_btn2, " "SYMBOL_EDIT" Manually  ");
	lv_obj_align(btn4, btn2, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 8, 0);
	lv_btn_set_action(btn4, LV_BTN_ACTION_CLICK, _create_mbox_clock_edit);
	lv_btn_set_state(btn4, LV_BTN_STATE_REL);

	label_txt2 = lv_label_create(sw_h2, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"#FF8000 Clock (HOS):# #C7EA46 Change clock offset based on what HOS uses.#\n"
		"#FF8000 Manually:# #C7EA46 Edit the clock offset manually.#");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 4);

	label_sep = lv_label_create(sw_h3, NULL);
	lv_label_set_static_text(label_sep, "");

	// Create Backup/Restore Verification list.
	label_txt = lv_label_create(sw_h3, NULL);
	lv_label_set_static_text(label_txt, SYMBOL_MODULES_ALT" Data Verification");
	lv_obj_set_style(label_txt, th->label.prim);
	lv_obj_align(label_txt, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);

	lv_obj_t *ddlist2 = lv_ddlist_create(sw_h3, NULL);
	lv_obj_set_top(ddlist2, true);
	lv_ddlist_set_draw_arrow(ddlist2, true);
	lv_ddlist_set_options(ddlist2,
		"Off (Fastest)\n"
		"Sparse (Fast)    \n"
		"Full (Slow)\n"
		"Full (Hashes)");
	lv_ddlist_set_selected(ddlist2, n_cfg.verification);
	lv_obj_align(ddlist2, label_txt, LV_ALIGN_OUT_RIGHT_MID, LV_DPI * 3 / 8, 0);
	lv_ddlist_set_action(ddlist2, _data_verification_action);

	label_txt2 = lv_label_create(sw_h3, NULL);
	lv_label_set_static_text(label_txt2, "Set the type of data verification done for backup and restore.\n"
		"Can be canceled without losing the backup/restore.\n");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, label_txt, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 4);

	line_sep = lv_line_create(sw_h3, line_sep);
	lv_obj_align(line_sep, label_txt2, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 4);

	lv_obj_t *btn5 = lv_btn_create(sw_h3, NULL);
	lv_obj_t *label_btn5 = lv_label_create(btn5, NULL);
	lv_btn_set_fit(btn5, true, true);
	lv_label_set_static_text(label_btn5, SYMBOL_EDIT" Save Options");
	lv_obj_align(btn5, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI * 31 / 21, LV_DPI * 6 / 8);
	lv_btn_set_action(btn5, LV_BTN_ACTION_CLICK, _save_nyx_options_action);

	lv_obj_set_top(l_cont, true); // Set the ddlist container at top.
	lv_obj_set_parent(ddlist, l_cont); // Reorder ddlist.
	lv_obj_set_top(ddlist, true);
	lv_obj_set_top(ddlist2, true);

	return LV_RES_OK;
}

void create_tab_options(lv_theme_t *th, lv_obj_t *parent)
{
	lv_page_set_scrl_layout(parent, LV_LAYOUT_PRETTY);

	static lv_style_t h_style;
	lv_style_copy(&h_style, &lv_style_transp);
	h_style.body.padding.inner = 0;
	h_style.body.padding.hor = LV_DPI - (LV_DPI / 4);
	h_style.body.padding.ver = LV_DPI / 6;

	// Create containers to keep content inside.
	lv_obj_t * sw_h2 = lv_cont_create(parent, NULL);
	lv_cont_set_style(sw_h2, &h_style);
	lv_cont_set_fit(sw_h2, false, true);
	lv_obj_set_width(sw_h2, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(sw_h2, false);
	lv_cont_set_layout(sw_h2, LV_LAYOUT_OFF);

	lv_obj_t * sw_h3 = lv_cont_create(parent, NULL);
	lv_cont_set_style(sw_h3, &h_style);
	lv_cont_set_fit(sw_h3, false, true);
	lv_obj_set_width(sw_h3, (LV_HOR_RES / 9) * 4);
	lv_obj_set_click(sw_h3, false);
	lv_cont_set_layout(sw_h3, LV_LAYOUT_OFF);

	lv_obj_t * l_cont = lv_cont_create(sw_h2, NULL);
	lv_cont_set_style(l_cont, &lv_style_transp_tight);
	lv_cont_set_fit(l_cont, true, true);
	lv_obj_set_click(l_cont, false);
	lv_cont_set_layout(l_cont, LV_LAYOUT_OFF);
	lv_obj_set_opa_scale(l_cont, LV_OPA_40);

	lv_obj_t *label_sep = lv_label_create(sw_h2, NULL);
	lv_label_set_static_text(label_sep, "");

	// Create Auto Boot button.
	lv_obj_t *btn = lv_btn_create(sw_h2, NULL);
	if (hekate_bg)
	{
		lv_btn_set_style(btn, LV_BTN_STYLE_REL, &btn_transp_rel);
		lv_btn_set_style(btn, LV_BTN_STYLE_PR, &btn_transp_pr);
		lv_btn_set_style(btn, LV_BTN_STYLE_TGL_REL, &btn_transp_tgl_rel);
		lv_btn_set_style(btn, LV_BTN_STYLE_TGL_PR, &btn_transp_tgl_pr);
	}
	lv_btn_set_layout(btn, LV_LAYOUT_OFF);
	lv_obj_t *label_btn = lv_label_create(btn, NULL);
	lv_label_set_recolor(label_btn, true);
	lv_btn_set_fit(btn, true, true);
	lv_btn_set_toggle(btn, true);
	lv_label_set_static_text(label_btn, SYMBOL_GPS" Auto Boot #00FFC9   ON #");
	lv_btn_set_action(btn, LV_BTN_ACTION_CLICK, _autoboot_hide_delay_action);
	lv_obj_align(btn, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, -LV_DPI / 18 + 6);
	lv_btn_set_fit(btn, false, false);
	autoboot_btn = btn;

	lv_obj_t *label_txt2 = lv_label_create(sw_h2, NULL);
	lv_label_set_static_text(label_txt2, "Choose which boot entry or payload to automatically boot\nwhen injecting.");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 3 - 4);

	lv_obj_t *line_sep = lv_line_create(sw_h2, NULL);
	static const lv_point_t line_pp[] = { {0, 0}, { LV_HOR_RES - (LV_DPI - (LV_DPI / 4)) * 2, 0} };
	lv_line_set_points(line_sep, line_pp, 2);
	lv_line_set_style(line_sep, th->line.decor);
	lv_obj_align(line_sep, label_txt2, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 4);

	// Create Boot time delay list.
	lv_obj_t *label_txt = lv_label_create(l_cont, NULL);
	lv_label_set_static_text(label_txt, SYMBOL_CLOCK" Boot Time Delay  ");
	lv_obj_set_style(label_txt, th->label.prim);
	lv_obj_align(label_txt, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);

	lv_obj_t *ddlist = lv_ddlist_create(l_cont, NULL);
	lv_obj_set_top(ddlist, true);
	lv_ddlist_set_draw_arrow(ddlist, true);
	lv_ddlist_set_options(ddlist,
		"No bootlogo    \n"
		"1 second\n"
		"2 seconds\n"
		"3 seconds\n"
		"4 seconds\n"
		"5 seconds\n"
		"6 seconds");
	lv_ddlist_set_selected(ddlist, 3);
	lv_obj_align(ddlist, label_txt, LV_ALIGN_OUT_RIGHT_MID, LV_DPI / 4, 0);
	lv_ddlist_set_action(ddlist, _autoboot_delay_action);
	lv_ddlist_set_selected(ddlist, h_cfg.bootwait);

	if (hekate_bg)
	{
		lv_ddlist_set_style(ddlist, LV_DDLIST_STYLE_BG, &ddlist_transp_bg);
		lv_ddlist_set_style(ddlist, LV_DDLIST_STYLE_BGO, &ddlist_transp_bg);
		lv_ddlist_set_style(ddlist, LV_DDLIST_STYLE_PR, &ddlist_transp_sel);
		lv_ddlist_set_style(ddlist, LV_DDLIST_STYLE_SEL, &ddlist_transp_sel);
	}

	label_txt2 = lv_label_create(l_cont, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"Set how long to show bootlogo when autoboot is enabled.\n"
		"#C7EA46 You can press# #FF8000 VOL-# #C7EA46 during that time to enter hekate's menu.#\n");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, label_txt, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 4);

	line_sep = lv_line_create(sw_h2, line_sep);
	lv_obj_align(line_sep, label_txt2, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 4);

	// Create Auto NoGC button.
	lv_obj_t *btn2 = lv_btn_create(sw_h2, NULL);
	nyx_create_onoff_button(th, sw_h2, btn2, SYMBOL_SHRK" Auto NoGC", auto_nogc_toggle, true);
	lv_obj_align(btn2, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 10);

	label_txt2 = lv_label_create(sw_h2, NULL);
	lv_label_set_recolor(label_txt2, true);
	lv_label_set_static_text(label_txt2,
		"It checks fuses and applies the patch automatically\n"
		"if higher firmware. It is now a global config and set\n"
		"at auto by default. (ON: Auto)\n\n\n");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn2, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 12);

	label_sep = lv_label_create(sw_h3, NULL);
	lv_label_set_static_text(label_sep, "");

	// Create Auto HOS Power Off button.
	lv_obj_t *btn3 = lv_btn_create(sw_h3, NULL);
	nyx_create_onoff_button(th, sw_h3, btn3, SYMBOL_POWER" Auto HOS Power Off", auto_hos_poweroff_toggle, true);
	lv_obj_align(btn3, label_sep, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

	label_txt2 = lv_label_create(sw_h3, NULL);
	lv_label_set_static_text(label_txt2,
		"When Shutdown is used from HOS, the device wakes up after\n"
		"15s. Enable this to automatically power off on the next\npayload injection.");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, btn3, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 12);

	line_sep = lv_line_create(sw_h3, line_sep);
	lv_obj_align(line_sep, label_txt2, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 4);

	// Create Backlight slider.
	label_txt = lv_label_create(sw_h3, NULL);
	lv_label_set_static_text(label_txt, SYMBOL_BRIGHTNESS" Backlight");
	lv_obj_set_style(label_txt, th->label.prim);
	lv_obj_align(label_txt, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);

	lv_obj_t * slider = lv_slider_create(sw_h3, NULL);
	lv_obj_set_width(slider, LV_DPI * 80 / 34);
	//lv_obj_set_height(slider, LV_DPI * 4 / 10);
	lv_bar_set_range(slider, 30, 220);
	lv_bar_set_value(slider, h_cfg.backlight);
	lv_slider_set_action(slider, _slider_brightness_action);
	lv_obj_align(slider, label_txt, LV_ALIGN_OUT_RIGHT_MID, LV_DPI * 20 / 15, 0);

	label_txt2 = lv_label_create(sw_h3, NULL);
	lv_label_set_static_text(label_txt2, "Set backlight brightness.\n\n");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, label_txt, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 4);

	line_sep = lv_line_create(sw_h3, line_sep);
	lv_obj_align(line_sep, label_txt2, LV_ALIGN_OUT_BOTTOM_LEFT, -(LV_DPI / 4), LV_DPI / 4);

	// Create Backup/Restore Verification list.
	label_txt = lv_label_create(sw_h3, NULL);
	lv_label_set_static_text(label_txt, SYMBOL_MODULES_ALT" Data Verification");
	lv_obj_set_style(label_txt, th->label.prim);
	lv_obj_align(label_txt, line_sep, LV_ALIGN_OUT_BOTTOM_LEFT, LV_DPI / 4, LV_DPI / 4);

	lv_obj_t *ddlist2 = lv_ddlist_create(sw_h3, NULL);
	lv_obj_set_top(ddlist2, true);
	lv_ddlist_set_draw_arrow(ddlist2, true);
	lv_ddlist_set_options(ddlist2,
		"Off (Fastest)\n"
		"Sparse (Fast)    \n"
		"Full (Slow)\n"
		"Full (Hashes)");
	lv_ddlist_set_selected(ddlist2, n_cfg.verification);
	lv_obj_align(ddlist2, label_txt, LV_ALIGN_OUT_RIGHT_MID, LV_DPI * 3 / 8, 0);
	lv_ddlist_set_action(ddlist2, _data_verification_action);

	if (hekate_bg)
	{
		lv_ddlist_set_style(ddlist2, LV_DDLIST_STYLE_BG, &ddlist_transp_bg);
		lv_ddlist_set_style(ddlist2, LV_DDLIST_STYLE_BGO, &ddlist_transp_bg);
		lv_ddlist_set_style(ddlist2, LV_DDLIST_STYLE_PR, &ddlist_transp_sel);
		lv_ddlist_set_style(ddlist2, LV_DDLIST_STYLE_SEL, &ddlist_transp_sel);
	}

	label_txt2 = lv_label_create(sw_h3, NULL);
	lv_label_set_static_text(label_txt2, "Set the type of data verification done for backup and restore.\n"
		"Can be canceled without losing the backup/restore.\n\n\n\n");
	lv_obj_set_style(label_txt2, &hint_small_style);
	lv_obj_align(label_txt2, label_txt, LV_ALIGN_OUT_BOTTOM_LEFT, 0, LV_DPI / 4);

	// Set default loaded states.
	if (h_cfg.autohosoff)
		lv_btn_set_state(btn3, LV_BTN_STATE_TGL_REL);
	if (h_cfg.autonogc)
		lv_btn_set_state(btn2, LV_BTN_STATE_TGL_REL);

	nyx_generic_onoff_toggle(btn2);
	nyx_generic_onoff_toggle(btn3);
	_autoboot_hide_delay_action(btn);

	lv_obj_set_top(l_cont, true); // Set the ddlist container at top.
	lv_obj_set_parent(ddlist, l_cont); // Reorder ddlist.
	lv_obj_set_top(ddlist, true);
	lv_obj_set_top(ddlist2, true);
}
