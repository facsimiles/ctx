"""
Personal State Script
=====================
"""
import color
import os
import personal_state
import simple_menu

states = [
    ("No State", personal_state.NO_STATE),
    ("No Contact", personal_state.NO_CONTACT),
    ("Chaos", personal_state.CHAOS),
    ("Communication", personal_state.COMMUNICATION),
    ("Camp", personal_state.CAMP),
]


class StateMenu(simple_menu.Menu):
    color_sel = color.WHITE

    def on_scroll(self, item, index):
        personal_state.set(item[1], False)

    def on_select(self, item, index):
        personal_state.set(item[1], True)
        os.exit()

    def entry2name(self, value):
        return value[0]

    def draw_entry(self, item, index, offset):
        if item[1] == personal_state.NO_CONTACT:
            self.color_1 = color.RED
            self.color_2 = color.RED
            self.color_text = color.WHITE
        elif item[1] == personal_state.CHAOS:
            self.color_1 = color.CHAOSBLUE
            self.color_2 = color.CHAOSBLUE
            self.color_text = color.CHAOSBLUE_DARK
        elif item[1] == personal_state.COMMUNICATION:
            self.color_1 = color.COMMYELLOW
            self.color_2 = color.COMMYELLOW
            self.color_text = color.COMMYELLOW_DARK
        elif item[1] == personal_state.CAMP:
            self.color_1 = color.CAMPGREEN
            self.color_2 = color.CAMPGREEN
            self.color_text = color.CAMPGREEN_DARK
        else:
            self.color_1 = color.Color(100, 100, 100)
            self.color_2 = color.Color(100, 100, 100)
            self.color_text = color.Color(200, 200, 200)

        super().draw_entry(item, index, offset)


if __name__ == "__main__":
    StateMenu(states).run()
