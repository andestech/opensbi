/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Atish Patra <atish.patra@wdc.com>
 */

#include <sbi/sbi_console.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_string.h>

static inline char *sbi_platform_feature_id2string(unsigned long feature)
{
	char *fstr = NULL;

	if (!feature)
		return NULL;

	switch (feature) {
	case SBI_PLATFORM_HAS_MFAULTS_DELEGATION:
		fstr = "medeleg";
		break;
	default:
		break;
	}

	return fstr;
}

void sbi_platform_get_features_str(const struct sbi_platform *plat,
				   char *features_str, int nfstr)
{
	unsigned long features, feat = 1UL;
	char *temp;
	int offset = 0;

	if (!plat || !features_str || !nfstr)
		return;
	sbi_memset(features_str, 0, nfstr);

	features = sbi_platform_get_features(plat);
	if (!features)
		goto done;

	do {
		if (features & feat) {
			temp = sbi_platform_feature_id2string(feat);
			if (temp) {
				int len = sbi_snprintf(features_str + offset,
						       nfstr - offset,
						       "%s,", temp);
				if (len < 0)
					break;

				if (offset + len >= nfstr) {
					/* No more space for features */
					offset = nfstr;
					break;
				} else
					offset = offset + len;
			}
		}
		feat = feat << 1;
	} while (feat <= SBI_PLATFORM_HAS_LAST_FEATURE);

done:
	if (offset)
		features_str[offset - 1] = '\0';
	else
		sbi_strncpy(features_str, "none", nfstr);
}
