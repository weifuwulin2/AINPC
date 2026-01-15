# Known Bugs & Issues Log

This document tracks known issues, their status, and potential fixes.

## [Critical] Cowardly NPC Attacks under High Threat
**Status**: Open (High Priority)
**Date Logged**: 2026-01-15
**Component**: UtilityAI / Prompt Engineering

### Description
A "Cowardly Merchant" NPC (High Neuroticism, Low Agreeableness) is initiating `Action_Attack` when sensing a zombie (`Perceived_Threat` high).
**Expected Behavior**: The NPC should `Flee` or `Beg`.
**Actual Behavior**: The NPC attacks.

### Potential Causes (Hypothesis)
1.  **Intention Override Misfire**:
    *   The LLM prompt might not be constrained enough. Even though the persona is "Coward", the LLM might output `"Intention": "Attack"` in a panic situation.
    *   Due to the new `Intention Override` mechanism (x3.0 multiplier), this single token output forces the Utility AI to ignore physical weakness and personality traits.
2.  **Action Configuration**:
    *   `Action_Attack` might have `Perceived_Threat` as a positive motivation input (Linear Curve), causing high threat to *increase* attack desire regardless of personality.
    *   The "Coward" personality weight for `Indignity` (Anger) might not be low enough to suppress the base score.

### Next Steps (Investigation Plan)
1.  **Check LLM Logs**: Verify if the LLM actually output `"Intention": "Attack"`.
2.  **Check Action Config**: Verify input curves for `Action_Attack`. It should NOT use `Perceived_Threat` as a positive driver unless `FightOrFlight` logic is explicitly handled.
3.  **Tweak Weights/Prompt**:
    *   If LLM is at fault: Add strict system instruction: *"If [Personality=Coward] AND [Threat=High], YOU MUST INTEND TO FLEE."*
    *   If Utility is at fault: Reduce `BaseReward` of Attack or adjust `Neuroticism` impact on Aggression.
