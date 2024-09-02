/* *****************************************************************************
 * %{QMAKE_PROJECT_NAME}
 * Copyright (c) %YEAR% killerbee
 *
 * This program is free software: you can redistribute it and/or modify
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * ****************************************************************************/

#include "field.hpp"
#include "layoutspecs.hpp"

void field::finalize_nodes() {
	for (const auto y : range(height())) {
		for (const auto x : range(width)) {
			if (auto p = reg_node_by_location(x, y); useful(p)) {
				// safe because reg_node_by_location returns nullptr on OOB
				p->neighbors[left] = reg_node_by_location(x - 1, y);
				p->neighbors[right] = reg_node_by_location(x + 1, y);
				p->neighbors[up] = reg_node_by_location(x, y - 1);
				p->neighbors[down] = reg_node_by_location(x, y + 1);
			}
		}
	}

	for (const auto x : range(in_nodes_offset, out_nodes_offset)) {
		auto p = nodes[x].get();
		assert(valid(p));
		auto n = reg_node_by_location(p->x, 0);
		assert(valid(n));
		p->neighbors[down] = n;
		n->neighbors[up] = p;
	}

	for (const auto x : range(out_nodes_offset, nodes.size())) {
		auto p = nodes[x].get();
		assert(valid(p));
		auto n = reg_node_by_location(p->x, height() - 1);
		assert(valid(n));
		p->neighbors[up] = n;
		n->neighbors[down] = p;
	}

	for (auto& p : nodes) {
		if (useful(p.get())) {
			bool connected = false;
			for (auto& n : p->neighbors) {
				if (useful(n)) {
					connected = true;
				} else {
					n = nullptr;
				}
			}
			// nodes with HCF need to be simulated even if they are isolated
			if (connected
			    or (p->type() == node::T21
			        and static_cast<const T21*>(p.get())->has_instr(instr::hcf))) {
				log_debug("node at (", p->x, ',', p->y, ") marked useful");
				nodes_to_sim.push_back(p.get());
			}
		}
	}
}

std::size_t field::instructions() const {
	std::size_t ret{};
	for (auto it = begin(); it != end_regular(); ++it) {
		auto p = it->get();
		if (p->type() == node::T21) {
			ret += static_cast<T21*>(p)->code.size();
		}
	}
	return ret;
}

std::size_t field::nodes_used() const {
	std::size_t ret{};
	for (auto it = begin(); it != end_regular(); ++it) {
		auto p = it->get();
		if (p->type() == node::T21) {
			ret += not static_cast<T21*>(p)->code.empty();
		}
	}
	return ret;
}

std::string field::layout() const {
	auto h = height();
	std::string ret = concat(width, ' ', h, '\n');
	for (const auto y : range(h)) {
		for (const auto x : range(width)) {
			auto p = reg_node_by_location(x, y);
			if (p->type() == node::Damaged) {
				ret += 'D';
			} else if (p->type() == node::T21) {
				ret += 'C';
			} else if (p->type() == node::T30) {
				ret += 'S';
			}
		}
		ret += '\n';
	}

	for (auto it = begin_io(); it != end(); ++it) {
		auto p = it->get();

		if (p->type() == node::in) {
			auto in = static_cast<input_node*>(p);
			append(ret, 'I', p->x);
			if (not in->inputs.empty()) {
				append(ret, " [");
				for (auto v : in->inputs) {
					append(ret, v, ", ");
				}
				append(ret, "]");
			}
			append(ret, ' ');
		} else if (p->type() == node::out) {
			auto on = static_cast<output_node*>(p);
			append(ret, 'O', p->x);
			if (not on->outputs_expected.empty()) {
				append(ret, " [");
				for (auto v : on->outputs_expected) {
					append(ret, v, ", ");
				}
				append(ret, "]");
			}
			append(ret, ' ');
		} else if (p->type() == node::image) {
			auto im = static_cast<image_output*>(p);
			append(ret, 'V', p->x, " ", im->width, ',', im->height);
			if (not im->image_expected.blank()) {
				append(ret, " [", im->image_expected.write_text(false), "]");
			}
			append(ret, ' ');
		}
	}

	return ret;
}

constexpr std::string type_name(node::type_t t) {
	switch (t) {
	case node::T21:
		return "T21";
	case node::T30:
		return "T30";
	case node::in:
		return "in";
	case node::out:
		return "out";
	case node::image:
		return "image";
	case node::Damaged:
		return "Damaged";
	default:
		return "null";
	}
}

std::string field::machine_layout() const {
	std::ostringstream ret;
	ret << "{.nodes = {{\n";
	{
		auto it = begin();
		for (auto y = 0; y != 3; ++y) {
			ret << "\t\t\t{";
			for (auto x = 0; x != 4; ++x, ++it) {
				ret << type_name((*it)->type()) << ", ";
			}
			ret << "},\n";
		}
	}
	ret << "\t\t}}, .io = {{\n";
	std::array<std::array<node::type_t, 4>, 2> io;
	std::optional<std::pair<word_t, word_t>> image_size{};
	for (auto it = begin_io(); it != end(); ++it) {
		auto n = it->get();
		if (n->type() == node::in) {
			io[0][static_cast<std::size_t>(n->x)] = node::in;
		} else if (n->type() == node::out) {
			io[1][static_cast<std::size_t>(n->x)] = node::out;
		} else if (auto p = dynamic_cast<image_output*>(n)) {
			io[1][static_cast<std::size_t>(p->x)] = node::image;
			image_size = {p->width, p->height};
		}
	}
	for (auto t : io) {
		ret << "\t\t\t{{";
		for (auto n : t) {
			ret << type_name(n);
			ret << ", ";
		}
		ret << "}},\n";
	}
	ret << "\t}}}";
	return std::move(ret).str();

	using enum node::type_t;
	// clang-format off
	[[maybe_unused]] builtin_layout_spec s =
		{.nodes = {{
			{T21, T21, T21, T21},
			{T21, T21, T21, T21},
			{T21, T21, T21, T21},
		}},
	  .io = {{
			{{null, in, in, null, }},
			{{out, image, null, null, }}
		}}
	};
	// clang-format on
}

field field::clone() const {
	field ret;

	for (auto& n : nodes) {
		ret.nodes.push_back(n->clone());
	}

	ret.width = width;
	ret.in_nodes_offset = in_nodes_offset;
	ret.out_nodes_offset = out_nodes_offset;

	ret.finalize_nodes();

	return ret;
}
