# Update Log: Refined Sensory-Memory Integration

**Date:** 2026-01-09
**Module:** AINPC (Sensory & Memory Components)

## 1. Overview
This update refines how the AI perceives threats and how it retrieves memories related to those threats. The goal was to ensure that "seeing a Zombie" isn't just recorded as a generic visual event, but explicitly tagged as a dangerous event, and that the memory system can leverage these semantic tags for better retrieval.

## 2. Detailed Changes

### A. Sensory Component (`SensoryComponent.cpp`)
*   **Semantic Tag Upgrade:**
    *   Modified `HandleTargetPerceived` to check if a perceived actor has the `"Enemy"` tag.
    *   If an enemy is seen, the event `Verb` (Tag) is upgraded from `Perception.Vision` ("saw") to `Event.Danger` ("danger").
    *   **Impact:** This ensures that the memory recorded is semantically "I sensed danger" rather than just "I saw something," which triggers higher importance scoring in the Memory system logic.
*   **Importance Tuning:**
    *   Increased the base `Magnitude` (Importance) for perceiving enemies from `0.7` to `0.8`.

### B. Memory Component (`MemoryComponent.cpp`)
*   **Enhanced Retrieval Logic:**
    *   Updated `RetrieveRelevantMemories` to include **Tag Matching**.
    *   Previously, relevance was calculated solely by checking if the query words existed in the memory's *text description*.
    *   **New Logic:** The system now also checks if the query words match the memory's *Gameplay Tags*.
    *   **Weighting:** Matches against Tags provide a higher relevance score (+1.0) compared to matches against Description text (+0.5).
    *   **Impact:** If the AI (or LLM) asks for "Danger" or "Enemy", it will now reliably find memories tagged with `Event.Danger`, even if the description string doesn't explicitly contain the word "Danger".

## 3. Benefits
*   **Better Reactivity:** The AI is now more likely to "remember" and react to threats because they are tagged as such at the moment of perception.
*   **Improved Context:** When the overarching `CognitionComponent` queries for context (e.g., "What dangerous things happened?"), the memory retrieval is now semantically aware.
