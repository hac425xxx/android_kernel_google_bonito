/*
 * Copyright (c) 2014 Broadcom Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_irq.h>

#include <defs.h>
#include "debug.h"
#include "core.h"
#include "common.h"
#include "of.h"

void brcmf_of_probe(struct device *dev, struct brcmfmac_sdio_pd *sdio)
{
	struct device_node *np = dev->of_node;
	int irq;
	u32 irqf;
	u32 val;

	if (!np || !of_device_is_compatible(np, "brcm,bcm4329-fmac"))
		return;

	if (of_property_read_u32(np, "brcm,drive-strength", &val) == 0)
		sdio->drive_strength = val;

	/* make sure there are interrupts defined in the node */
	if (!of_find_property(np, "interrupts", NULL))
		return;

	irq = irq_of_parse_and_map(np, 0);
	if (!irq) {
		brcmf_err("interrupt could not be mapped\n");
		return;
	}
	irqf = irqd_get_trigger_type(irq_get_irq_data(irq));

	sdio->oob_irq_supported = true;
	sdio->oob_irq_nr = irq;
	sdio->oob_irq_flags = irqf;
}

/*
 * Copyright (C) 2017 Rafał Miłecki <rafal@milecki.pl>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <linux/of.h>
#include <net/cfg80211.h>
#include "core.h"

static void wiphy_freq_limits_apply(struct wiphy *wiphy,
				    struct ieee80211_freq_range *freq_limits,
				    unsigned int n_freq_limits)
{
	enum nl80211_band band;
	int i;
	if (WARN_ON(!n_freq_limits))
		return;
	for (band = 0; band < NUM_NL80211_BANDS; band++) {
		struct ieee80211_supported_band *sband = wiphy->bands[band];
		if (!sband)
			continue;
		for (i = 0; i < sband->n_channels; i++) {
			struct ieee80211_channel *chan = &sband->channels[i];
			if (chan->flags & IEEE80211_CHAN_DISABLED)
				continue;
		}
	}
}
void wiphy_read_of_freq_limits(struct wiphy *wiphy)
{
	struct device *dev = wiphy_dev(wiphy);
	struct device_node *np;
	struct property *prop;
	struct ieee80211_freq_range *freq_limits;
	unsigned int n_freq_limits;
	const __be32 *p;
	int len, i;
	int err = 0;
	if (!dev)
		return;
	np = dev_of_node(dev);
	if (!np)
		return;
	prop = of_find_property(np, "ieee80211-freq-limit", &len);
	if (!prop)
		return;
	if (!len || len % sizeof(u32) || len / sizeof(u32) % 2) {
		dev_err(dev, "ieee80211-freq-limit wrong format");
		return;
	}
	n_freq_limits = len / sizeof(u32) / 2;
	freq_limits = kcalloc(n_freq_limits, sizeof(*freq_limits), GFP_KERNEL);
	if (!freq_limits) {
		err = -ENOMEM;
		goto out_kfree;
	}
	p = NULL;
	for (i = 0; i < n_freq_limits; i++) {
		struct ieee80211_freq_range *limit = &freq_limits[i];
		p = of_prop_next_u32(prop, p, &limit->start_freq_khz);
		if (!p) {
			err = -EINVAL;
			goto out_kfree;
		}
		p = of_prop_next_u32(prop, p, &limit->end_freq_khz);
		if (!p) {
			err = -EINVAL;
			goto out_kfree;
		}
		if (!limit->start_freq_khz ||
		    !limit->end_freq_khz ||
		    limit->start_freq_khz >= limit->end_freq_khz) {
			err = -EINVAL;
			goto out_kfree;
		}
	}
	wiphy_freq_limits_apply(wiphy, freq_limits, n_freq_limits);
out_kfree:
	kfree(freq_limits);
	if (err)
		dev_err(dev, "Failed to get limits: %d\n", err);
}
EXPORT_SYMBOL(wiphy_read_of_freq_limits);
