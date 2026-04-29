(function() {
    function navigateToSearch(keyword) {
        var q = keyword.trim();
        if (q) {
            window.location.href = '/all_questions?q=' + encodeURIComponent(q);
        }
    }

    function initSearch() {
        var searchInput = document.getElementById('navbar-search');
        if (!searchInput) return;

        searchInput.addEventListener('keydown', function(e) {
            if (e.key === 'Enter') {
                e.preventDefault();
                navigateToSearch(this.value);
            }
        });
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', initSearch);
    } else {
        initSearch();
    }
})();
