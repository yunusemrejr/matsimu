---
name: intuitive-design-expert
description: Designs scientific applications so both domain experts and general users can understand and use them. Applies dual-audience UX, progressive disclosure, plain-language defaults, and clear scientific visualization. Use when designing or reviewing scientific UIs, simulation interfaces, data-heavy apps, or when the user mentions accessibility for non-experts, citizen science, or "scientists and normal people."
---

# Intuitive Design Expert

## Goal

Scientific apps must serve two audiences without compromise: **experts** who need precision and power, and **everyone else** who need clarity and confidence. Design so that newcomers succeed on first use and experts are never slowed down.

## Core Principles

1. **Clarity over cleverness** — Prefer obvious labels, consistent terms, and predictable flows. Avoid jargon in primary UI; offer it as optional detail.
2. **Progressive disclosure** — Default to simple. Expose advanced options, units, or parameters only when the user seeks them (toggles, "Advanced," "Expert mode").
3. **One primary path** — Every task should have a clear, minimal path that works. Alternative or advanced paths are additive, not required.
4. **Explain, don’t assume** — Short inline explanations, tooltips, or optional "What is this?" for concepts that non-experts might not know. Never assume prior domain knowledge for the default path.
5. **Feedback and safety** — Actions have visible results. Destructive or irreversible steps are explicit and confirmable. Units and scales are always visible where they matter.

## Dual-Audience Patterns

### Language and labels

- **Primary label**: Plain language (e.g. "Stiffness" or "How stiff the material is").
- **Secondary detail**: Domain term when relevant (e.g. "Young’s modulus" in tooltip or subtitle).
- Use one consistent term per concept in the UI; avoid mixing "stress" and "σ" unless one is clearly secondary (e.g. in a formula panel).

### Controls and parameters

- **Sensible defaults** that produce a valid, interpretable result so "Run" or "Simulate" works without configuration.
- **Ranges and units** visible on sliders/inputs (e.g. "0–100 GPa", "Temperature (°C)").
- **Presets or examples** (e.g. "Steel", "Rubber") to anchor non-experts and speed up experts.

### Complexity layers

- **Layer 1 (default)**: Minimal controls, one main action, clear outcome (e.g. "Run simulation", "View result").
- **Layer 2**: Optional parameters (e.g. "Show parameters", "Settings") with short explanations.
- **Layer 3**: Expert/advanced (e.g. "Expert mode", "Raw parameters") for full control without cluttering the main flow.

### Results and visualization

- **Interpretation over raw data first**: e.g. "Result: Pass / Fail" or "Within normal range" before showing full curves or numbers.
- **Axis labels and units** on every chart; avoid unlabeled or symbol-only axes in the default view.
- **Color and legend**: Use colorblind-friendly palettes; never rely on color alone to convey critical meaning. Provide text/legend.

## Scientific Accuracy

- Do not sacrifice correctness for simplicity. Defaults and explanations must be scientifically sound.
- When simplifying (e.g. "stiffness" instead of "Young’s modulus"), ensure the underlying model and units are still correct and documented for experts.
- Warnings or limits (e.g. "Valid for small strains") should be visible where they apply, without blocking the main flow.

## Anti-Patterns to Avoid

- **Jargon-first UI** — Leading with symbols (σ, ε, ∂) or technical names before the user has chosen an expert mode.
- **No default path** — Forcing all users to configure everything before any result.
- **Hidden units** — Numbers without units or with units only in a separate "advanced" panel.
- **Single level of depth** — Either only "toy" controls or only raw parameters; no middle layer.
- **Silent failure** — Errors that are vague ("Error") or only in logs; users need clear, actionable messages.
- **Assumed expertise** — Requiring the user to know what "convergence" or "mesh size" means before they can get a first result.

## Checklist for New Screens or Flows

- [ ] A new user can complete the primary task without reading docs.
- [ ] Every important control has a visible label and, where helpful, a short explanation or tooltip.
- [ ] Units and ranges are shown for numeric inputs that affect results.
- [ ] There is a safe, sensible default that produces a valid outcome.
- [ ] Expert-oriented options are available but not required (progressive disclosure).
- [ ] Results are shown in a way that supports both quick interpretation and deeper inspection.
- [ ] Error and validation messages are clear and suggest what to do next.

## When to Apply This Skill

- Designing or refactoring UI for simulations, data analysis, or scientific tools.
- Reviewing wireframes, mockups, or UI copy for scientific apps.
- Adding new parameters, views, or workflows that might confuse non-experts.
- User says "scientists and normal citizens," "everyone should understand," "accessible," or "intuitive" in the context of a scientific app.
