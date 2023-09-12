// Copyright 2023 Intrinsic Innovation LLC
// Intrinsic Proprietary and Confidential
// Provided subject to written agreement between the parties.

// Package listutil contains utils for commands that list released skills.
package listutil

import (
	"encoding/json"
	"fmt"
	"sort"
	"strings"

	skillcatalogpb "intrinsic/skills/catalog/proto/skill_catalog_go_grpc_proto"
	spb "intrinsic/skills/proto/skills_go_proto"
)

// Custom proto->json conversion to handle fields like the update timestamp.
type SkillDescription struct {
	Name         string `json:"name,omitempty"`
	Vendor       string `json:"vendor,omitempty"`
	PackageName  string `json:"packageName,omitempty"`
	Version      string `json:"version,omitempty"`
	UpdateTime   string `json:"updateTime,omitempty"`
	ID           string `json:"id,omitempty"`
	IDVersion    string `json:"idVersion,omitempty"`
	ReleaseNotes string `json:"releaseNotes,omitempty"`
	DocString    string `json:"docString,omitempty"`
}

// SkillDescriptions wraps the required data for the output of skill list commands.
type SkillDescriptions struct {
	Skills []SkillDescription `json:"skills"`
}

// SkillDescriptionsFromSkillMetas creates a SkillDescriptions instance from SkillMeta protos
func SkillDescriptionsFromSkillMetas(skillMetas []*skillcatalogpb.SkillMeta) *SkillDescriptions {
	out := SkillDescriptions{Skills: make([]SkillDescription, len(skillMetas))}

	for i, skillMeta := range skillMetas {
		out.Skills[i] = SkillDescription{
			Name:         skillMeta.GetName(),
			Vendor:       skillMeta.GetVendor(),
			PackageName:  skillMeta.GetPackageName(),
			Version:      skillMeta.GetVersion(),
			UpdateTime:   skillMeta.GetUpdateTime().AsTime().String(),
			ID:           skillMeta.GetId(),
			IDVersion:    skillMeta.GetIdVersion(),
			ReleaseNotes: skillMeta.GetReleaseNotes(),
			DocString:    skillMeta.GetDocString(),
		}
	}

	return &out
}

// SkillDescriptionsFromSkills creates a SkillDescriptions instance from Skill protos
func SkillDescriptionsFromSkills(skills []*spb.Skill) *SkillDescriptions {
	out := SkillDescriptions{Skills: make([]SkillDescription, len(skills))}

	for i, skill := range skills {
		out.Skills[i] = SkillDescription{
			Name:        skill.GetSkillName(),
			PackageName: skill.GetPackageName(),
			ID:          skill.GetId(),
			IDVersion:   skill.GetIdVersion(),
			DocString:   skill.GetDocString(),
		}
	}

	return &out
}

// MarshalJSON converts a SkillDescription to a byte slice.
func (sd SkillDescriptions) MarshalJSON() ([]byte, error) {
	return json.Marshal(struct {
		Skills []SkillDescription `json:"skills"`
	}{Skills: sd.Skills})
}

// String converts a SkillDescription to a string
func (sd SkillDescriptions) String() string {
	lines := []string{}
	for _, skill := range sd.Skills {
		lines = append(lines, fmt.Sprintf("%s", skill.IDVersion))
	}
	sort.Strings(lines)
	return strings.Join(lines, "\n")
}